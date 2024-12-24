#include "Card.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

ACard::ACard()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACard::BeginPlay()
{
    PrintCardDetails();
}

void ACard::InitializeCard(const FCardData& NewCardData)
{
    CardData = NewCardData;
    UE_LOG(LogTemp, Log, TEXT("Card Initialized: Name = %s, Ability = %d"), *CardData.Name, static_cast<uint8>(CardData.Ability));
}

void ACard::PrintCardDetails() const
{
    FString AbilityString = UEnum::GetValueAsString(CardData.Ability);
    UE_LOG(LogTemp, Log, TEXT("Card Details:\nName: %s\nDescription: %s\nValue: %d\nAbility: %s\nQuantity: %d"),
        *CardData.Name,
        *CardData.Description,
        CardData.Value,
        *AbilityString,
        CardData.Quantity);
}
