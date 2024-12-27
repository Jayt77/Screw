#include "CardManager.h"
#include "Card.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ACardManager::ACardManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACardManager::BeginPlay()
{
    Super::BeginPlay();

    // Initialize the deck and deal cards
    //InitializeDeck();
    //ShuffleDeck();
    //DealCardsToPlayers();

    // Start the first player's turn
    //AdvanceTurn();
}

void ACardManager::InitializeDeck()
{
    if (!CardDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("CardManager: CardDataTable is not set!"));
        return;
    }

    static const FString ContextString(TEXT("Card Data Context"));

    TArray<FName> RowNames = CardDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FCardData* RowData = CardDataTable->FindRow<FCardData>(RowName, ContextString);
        if (RowData)
        {
            for (int32 i = 0; i < RowData->Quantity; i++)
            {
                Deck.Add(*RowData);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Initialized Deck with %d card entries."), Deck.Num());
}

void ACardManager::ShuffleDeck()
{
    if (Deck.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CardManager: Cannot shuffle an empty deck!"));
        return;
    }

    for (int32 i = Deck.Num() - 1; i > 0; i--)
    {
        int32 SwapIndex = FMath::RandRange(0, i);
        Deck.Swap(i, SwapIndex);
    }

    UE_LOG(LogTemp, Log, TEXT("Deck shuffled."));
}

ACard* ACardManager::SpawnCardAtLocation_Implementation(const FCardData& CardData, const FTransform& Transform, AController* Controller)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Controller;
    ACard* NewCard = GetWorld()->SpawnActor<ACard>(CardClass, Transform, SpawnParams);

    if (IsValid(NewCard))
    {
        NewCard->InitializeCard(CardData);
        UE_LOG(LogTemp, Log, TEXT("Spawned card: %s at Location: %s"), *CardData.Name, *Transform.GetLocation().ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn card: %s"), *CardData.Name);
    }
    return NewCard;
}

void ACardManager::DealCardsToPlayers_Implementation()
{
    // Load all Player Controllers
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AController::StaticClass(), (TArray<AActor*>&)Controllers);

    if (Deck.Num() < CardsPerPlayer * Controllers.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Not enough cards in the deck to deal to all players!"));
        return;
    }

    for (AController* Controller : Controllers)
    {
        if (!IsValid(Controller) || !IsValid(Controller->GetPawn()))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid Controller or Pawn detected. Skipping."));
            continue;
        }

        APawn* PlayerPawn = Controller->GetPawn();
        FVector PlayerLocation = PlayerPawn->GetActorLocation();
        FVector ForwardVector = PlayerPawn->GetActorForwardVector();
        FVector RightVector = PlayerPawn->GetActorRightVector();

        FVector GridOrigin = PlayerLocation + (ForwardVector * ForwardOffset) + FVector(0.f, 0.f, VerticalOffset);
        GridOrigin -= RightVector * ((CardsPerPlayer - 1) * CardSpacing / 2);

        FPlayerHand PlayerHand;

        for (int32 i = 0; i < CardsPerPlayer; i++)
        {
            if (Deck.Num() == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("Deck ran out of cards while dealing to player %s!"), *Controller->GetName());
                break;
            }

            FCardData CardData = Deck.Pop();
            FVector CardPosition = GridOrigin + (RightVector * i * CardSpacing);
            FRotator CardRotation = ForwardVector.Rotation();
            CardRotation.Yaw = -ForwardVector.Rotation().Yaw;

            FTransform CardTransform(CardRotation, CardPosition);
            ACard* SpawnedCard = SpawnCardAtLocation(CardData, CardTransform, Controller);

            if (IsValid(SpawnedCard))
            {
                PlayerHand.Cards.Add(SpawnedCard);
            }
        }
        PlayerHands.Add(Controller, PlayerHand);
    }

    UE_LOG(LogTemp, Log, TEXT("All players have been dealt their cards at proper positions."));
}

void ACardManager::GrantCard(AController* Controller, ACard* Card, FTransform Transform) {
    if (!IsValid(Controller) || !IsValid(Controller->GetPawn()) || !IsValid(Card))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Controller or Pawn or card detected."));
        return;
    }

    FCardData CardData = Card->CardData;
    ACard* SpawnedCard = SpawnCardAtLocation(CardData, Transform, Controller);

    if (IsValid(SpawnedCard))
    {
        GetPlayerHand(Controller).Add(SpawnedCard);
    }
}

ACard* ACardManager::GrantCardFromDeck(AController* Controller, FTransform Transform) {
    if (!IsValid(Controller) || !IsValid(Controller->GetPawn()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Controller or Pawn detected."));
        return nullptr;
    }

    if (Deck.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Deck ran out of cards, regenerating!"));
        InitializeDeck();
        ShuffleDeck();
    }

    FCardData CardData = Deck.Pop();
    ACard* SpawnedCard = SpawnCardAtLocation(CardData, Transform, Controller);

    if (IsValid(SpawnedCard))
    {
        GetPlayerHand(Controller).Add(SpawnedCard);
    }

    return SpawnedCard;
}

void ACardManager::AdvanceTurn_Implementation()
{
    if (Controllers.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No players to take turns!"));
        return;
    }

    CurrentTurnIndex = (CurrentTurnIndex + 1) % Controllers.Num();
    CurrentTurnPlayer = Controllers[CurrentTurnIndex];

    UE_LOG(LogTemp, Log, TEXT("It's now Player %s's turn."), *CurrentTurnPlayer->GetName());
}

TArray<ACard*> ACardManager::GetPlayerHand(AController* Controller)
{
    if (PlayerHands.Contains(Controller))
    {
        return PlayerHands[Controller].Cards;
    }

    UE_LOG(LogTemp, Warning, TEXT("Controller not found in PlayerHands map!"));
    return TArray<ACard*>();
}

void ACardManager::PlayCard_Implementation(AController* Controller, int32 CardIndex)
{
    if (!PlayerHands.Contains(Controller))
    {
        UE_LOG(LogTemp, Warning, TEXT("Controller not found in PlayerHands map!"));
        return;
    }

    FPlayerHand& PlayerHand = PlayerHands[Controller];

    if (!PlayerHand.Cards.IsValidIndex(CardIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid card index selected by Player %s!"), *Controller->GetName());
        return;
    }

    ACard* PlayedCard = PlayerHand.Cards[CardIndex];
    if (IsValid(PlayedCard))
    {
        UE_LOG(LogTemp, Log, TEXT("Player %s played card: %s"), *Controller->GetName(), *PlayedCard->GetName());
        PlayedCard->PrintCardDetails();
        PlayerHand.Cards.RemoveAt(CardIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("The selected card is invalid!"));
    }
}
