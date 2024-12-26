#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Card.generated.h"

/** Enum for Card Abilities */
UENUM(BlueprintType)
enum class ECardAbility : uint8
{
    None UMETA(DisplayName = "None"),
    Selection UMETA(DisplayName = "Selection"),
    RevealOpponentOne UMETA(DisplayName = "Reveal Opponent One"),
    RevealSelfOne UMETA(DisplayName = "Reveal Self One"),
    SwapOneNoReveal UMETA(DisplayName = "Swap One No Reveal"),
    RevealOneEachOpponent UMETA(DisplayName = "Reveal One from Each Opponent"),
    DiscardOneSelf UMETA(DisplayName = "Discard One Self"),
    RevealOneAndSwap UMETA(DisplayName = "Reveal One and Swap")
};

/** Struct for Card Data */
USTRUCT(BlueprintType)
struct FCardData : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Data")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Data")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Data")
    int32 Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Data")
    UTexture* Texture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Data")
    ECardAbility Ability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Data")
    int32 Quantity;

    FCardData()
        : Name(TEXT("Default Card"))
        , Description(TEXT("No Description"))
        , Value(0)
        , Texture(nullptr)
        , Ability(ECardAbility::None)
        , Quantity(1)
    {
    }
};

UCLASS()
class SCREW_API ACard : public AActor
{
    GENERATED_BODY()

public:
    ACard();

protected:
    virtual void BeginPlay() override;

public:
    /** Initialize the card with specific data */
    UFUNCTION(BlueprintCallable, Category = "Card")
    void InitializeCard(const FCardData& NewCardData);

    /** Display card details in the log (for debugging) */
    UFUNCTION(BlueprintCallable, Category = "Card")
    void PrintCardDetails() const;

    /** Card Data Struct to hold the card's information */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card", meta = (AllowPrivateAccess = "true"))
    FCardData CardData;
};
