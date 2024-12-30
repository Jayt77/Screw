#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Card.h"
#include "CardManager.generated.h"

/** Struct to Wrap Player Hands */
USTRUCT(BlueprintType)
struct FPlayerHand
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Card Data")
    TArray<ACard*> Cards;

    FPlayerHand() {}
};

UCLASS()
class SCREW_API ACardManager : public AActor
{
    GENERATED_BODY()

public:
    ACardManager();

    /** Reference to the Card Data Table */
    UPROPERTY(EditDefaultsOnly, Category = "Card Settings")
    UDataTable* CardDataTable;

    /** Number of cards each player receives */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Settings")
    int32 CardsPerPlayer = 4;

    /** Horizontal Spacing Between Cards */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Settings")
    float HorizontalCardSpacing = 60.f;

    /** Vertical Spacing Between Cards */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Settings")
    float VerticalCardSpacing = 50.f;

    /** Forward Offset for Card Grid */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Settings")
    float ForwardOffset = 300.f;

    /** Vertical Offset for Card Grid */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Settings")
    float VerticalOffset = 50.f;

    UPROPERTY(EditDefaultsOnly, Category = "Card Settings")
    TSubclassOf<ACard> CardClass;

protected:
    virtual void BeginPlay() override;

    /** Initializes the deck with shuffled cards from the Data Table */
    UFUNCTION(BlueprintCallable, Category = "Card Management")
    void InitializeDeck();

    /** Deals cards to all players */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Card Management")
    void DealCardsToPlayers();

    /** Advances the turn to the next player */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Card Management")
    void AdvanceTurn();

    /** Shuffles the deck */
    UFUNCTION(BlueprintCallable, Category = "Card Management")
    void ShuffleDeck();

    /** Plays a card from a player's hand */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Card Management")
    void PlayCard(AController* Controller, int32 CardIndex);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Card Management")
    /** Spawns a single card at a specified location */
    ACard* SpawnCardAtLocation(const FCardData& CardData, const FTransform& Transform, AController* Controller);

    /** Retrieves the hand of a specific player */
    UFUNCTION(BlueprintCallable, Category = "Card Management")
    TArray<ACard*> GetPlayerHand(AController* Controller);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Settings")
    /** Current turn index */
    int32 CurrentTurnIndex = -1;

    UPROPERTY(BlueprintReadWrite, Category = "Card Settings")
    int32 TurnCount;

    UPROPERTY(BlueprintReadWrite, Category = "Card Settings")
    AController* CurrentTurnPlayer;

    UFUNCTION(BlueprintCallable, Category = "Card Settings")
    void GrantCard(AController* Controller, ACard* Card, FTransform Transform);

    UFUNCTION(BlueprintCallable, Category = "Card Settings")
    ACard* GrantCardFromDeck(AController* Controller, FTransform Transform);
    
private:
    /** The deck containing card data */
    UPROPERTY()
    TArray<FCardData> Deck;

    /** Hands for each player */
    UPROPERTY()
    TMap<AController*, FPlayerHand> PlayerHands;

    /** Array of connected Controllers */
    UPROPERTY()
    TArray<AController*> Controllers;

};
