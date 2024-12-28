// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "GBAHeaderViewAttributeVariableListItem.h"

#include "AttributeSet.h"
#include "Abilities/GBAAttributeSetBlueprintBase.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedStruct.h"
#include "Framework/SlateDelegates.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/StructureEditorUtils.h"
#include "LineEndings/GBALineEndings.h"
#include "Misc/EngineVersionComparison.h"
#include "Models/GBAAttributeSetWizardViewModel.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "FGBAHeaderViewAttributeVariableListItem"

FGBAHeaderViewAttributeVariableListItem::FGBAHeaderViewAttributeVariableListItem(const FProperty& InVarProperty, const TSharedPtr<FGBAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());

	const FString VarName = InVarProperty.GetAuthoredName();

	const FString ClampedPropertyTypename = TEXT("FGBAGameplayClampedAttributeData");
	
	// FGBAGameplayClampedAttributeData handling requires logic in PostGameplayEffectExecute handled by UGBAAttributeSetBlueprintBase
	const bool bSupportsClampedAttributeData = IsSupportingClampedAttributeData(InViewModel);
	const bool bIsClampedAttributeData = InVarProperty.GetCPPType() == ClampedPropertyTypename;

	// Format comment
	{
		FString Comment = InVarProperty.GetMetaData(FBlueprintMetadata::MD_Tooltip);
		if (Comment.IsEmpty())
		{
			Comment = FString::Printf(TEXT("%s Attribute"), *VarName);
			
			if (bIsClampedAttributeData && !bSupportsClampedAttributeData)
			{
				Comment += FString::Printf(TEXT("\n\n%s was a %s but handling of clamped properties requires"), *VarName, *ClampedPropertyTypename);
				Comment += TEXT("\na UGBAAttributeSetBlueprintBase parent class.");
				Comment += TEXT("\n\nPlease pick UGBAAttributeSetBlueprintBase for the parent class if you wish to generate the class");
				Comment += FString::Printf(TEXT("\nwith %s properties."), *ClampedPropertyTypename);
				Comment += TEXT("\n\nOr you can ignore this comment and later change or remove it in the generated C++ class.");
			}
		}

		FormatCommentString(Comment, RawItemString, RichTextString);
	}

	// Add the UPROPERTY specifiers
	// i.e. UPROPERTY(BlueprintReadWrite, Category="Variable Category")
	{
		const FString AdditionalSpecifiers = GetConditionalUPropertySpecifiers(InVarProperty);
		RawItemString += FString::Printf(TEXT("\nUPROPERTY(%s)"), *AdditionalSpecifiers);
		RichTextString += FString::Printf(TEXT("\n<%s>UPROPERTY</>(%s)"), *GBA::HeaderViewSyntaxDecorators::MacroDecorator, *AdditionalSpecifiers);
	}

	// Add the variable declaration line
	// i.e. Type VariableName;
	{
		FString Typename = GetCPPTypenameForProperty(&InVarProperty, /*bIsMemberProperty=*/true);
		const bool bLegalName = IsValidCPPIdentifier(VarName);

		if (bIsClampedAttributeData && !bSupportsClampedAttributeData)
		{
			// Convert FGBAGameplayClampedAttributeData to regular FGameplayAttributeData in case parent class is not
			// of type UGBAAttributeSetBlueprintBase, whose PostGameplayEffectExecute (or PreAttributeChange / PreAttributeBaseChange
			// if reworked)
			Typename = TEXT("FGameplayAttributeData");
		}

		const FString* IdentifierDecorator = &GBA::HeaderViewSyntaxDecorators::IdentifierDecorator;
		if (!bLegalName)
		{
			IllegalName = FName(VarName);
			IdentifierDecorator = &GBA::HeaderViewSyntaxDecorators::ErrorDecorator;
		}

		float BaseValue = 0.f;
		const TSharedPtr<FGameplayAttributeData> AttributeData = GetGameplayAttributeData(InVarProperty);
		if (AttributeData.IsValid())
		{
			BaseValue = AttributeData->GetBaseValue();
		}

		RawItemString += FString::Printf(TEXT("\n%s %s = %.2ff;"), *Typename, *VarName, BaseValue);
		RichTextString += FString::Printf(
			TEXT("\n<%s>%s</> <%s>%s</> = <%s>%.2ff</>;"),
			*GBA::HeaderViewSyntaxDecorators::TypenameDecorator,
			*Typename.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;")),
			**IdentifierDecorator,
			*VarName,
			*GBA::HeaderViewSyntaxDecorators::NumericLiteral,
			BaseValue
		);
	}

	// Add the attribute accessors macro
	if (const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get())
	{
		const FString ClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());

		const bool bLegalName = IsValidCPPIdentifier(VarName);
		const FString* IdentifierDecorator = &GBA::HeaderViewSyntaxDecorators::IdentifierDecorator;
		if (!bLegalName)
		{
			IllegalName = FName(VarName);
			IdentifierDecorator = &GBA::HeaderViewSyntaxDecorators::ErrorDecorator;
		}

		RawItemString += FString::Printf(TEXT("\nATTRIBUTE_ACCESSORS(%s, %s)"), *ClassName, *VarName);
		RichTextString += FString::Printf(
			TEXT("\n<%s>ATTRIBUTE_ACCESSORS</>(<%s>%s</>, <%s>%s</>)"),
			*GBA::HeaderViewSyntaxDecorators::MacroDecorator,
			*GBA::HeaderViewSyntaxDecorators::TypenameDecorator,
			*ClassName,
			**IdentifierDecorator,
			*VarName
		);
	}

	// indent item
	RawItemString.InsertAt(0, TEXT("\t"));
	RichTextString.InsertAt(0, TEXT("\t"));
	RawItemString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));
	RichTextString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));

	// normalize to platform newlines
	GBA::String::ToHostLineEndingsInline(RawItemString);
	GBA::String::ToHostLineEndingsInline(RichTextString);
}

FGBAHeaderViewListItemPtr FGBAHeaderViewAttributeVariableListItem::Create(const FProperty& InVarProperty, const TSharedPtr<FGBAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShareable(new FGBAHeaderViewAttributeVariableListItem(InVarProperty, InViewModel));
}


void FGBAHeaderViewAttributeVariableListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, const TWeakObjectPtr<UObject> InAsset)
{
	if (!IllegalName.IsNone())
	{
#if UE_VERSION_NEWER_THAN(5, 1, -1)
		InMenuBuilder.AddVerifiedEditableText(LOCTEXT("RenameItem", "Rename Variable"),
			LOCTEXT("RenameItemTooltip", "Renames this variable in the Blueprint\nThis variable name is not a legal C++ identifier."),
			FSlateIcon(),
			FText::FromName(IllegalName),
			FOnVerifyTextChanged::CreateSP(this, &FGBAHeaderViewAttributeVariableListItem::OnVerifyRenameTextChanged, InAsset),
			FOnTextCommitted::CreateSP(this, &FGBAHeaderViewAttributeVariableListItem::OnRenameTextCommitted, InAsset)
		);
#endif
	}
}

FString FGBAHeaderViewAttributeVariableListItem::GetConditionalUPropertySpecifiers(const FProperty& VarProperty)
{
	TArray<FString> PropertySpecifiers;

	PropertySpecifiers.Emplace(TEXT("BlueprintReadOnly"));

	if (VarProperty.HasMetaData(FBlueprintMetadata::MD_FunctionCategory))
	{
		PropertySpecifiers.Emplace(FString::Printf(
			TEXT("Category = \"%s\""),
			*VarProperty.GetMetaData(FBlueprintMetadata::MD_FunctionCategory)
		));
	}

	// Add on rep specifier if set to replicated
	if (VarProperty.HasAnyPropertyFlags(CPF_Net))
	{
		PropertySpecifiers.Emplace(FString::Printf(TEXT("ReplicatedUsing = OnRep_%s"), *VarProperty.GetAuthoredName()));
	}

	// TODO: Question about saving / loading
	// if (VarProperty.HasAnyPropertyFlags(CPF_SaveGame))
	// {
	// 	PropertySpecifiers.Emplace(TEXT("SaveGame"));
	// }

	return FString::Join(PropertySpecifiers, TEXT(", "));
}

bool FGBAHeaderViewAttributeVariableListItem::OnVerifyRenameTextChanged(const FText& InNewName, FText& OutErrorText, const TWeakObjectPtr<UObject> WeakAsset) const
{
	if (!IsValidCPPIdentifier(InNewName.ToString()))
	{
		OutErrorText = InvalidCPPIdentifierErrorText;
		return false;
	}

	UObject* Asset = WeakAsset.Get();

	if (const UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
	{
		FKismetNameValidator NameValidator(Blueprint, NAME_None, nullptr);
		const EValidatorResult Result = NameValidator.IsValid(InNewName.ToString());
		if (Result != EValidatorResult::Ok)
		{
			OutErrorText = GetErrorTextFromValidatorResult(Result);
			return false;
		}

		return true;
	}

	if (const UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(Asset))
	{
		return FStructureEditorUtils::IsUniqueVariableFriendlyName(Struct, InNewName.ToString());
	}

	return false;
}

void FGBAHeaderViewAttributeVariableListItem::OnRenameTextCommitted(const FText& CommittedText, const ETextCommit::Type TextCommitType, const TWeakObjectPtr<UObject> WeakAsset) const
{
	if (TextCommitType == ETextCommit::OnEnter)
	{
		if (UObject* Asset = WeakAsset.Get())
		{
			const FString& CommittedString = CommittedText.ToString();
			if (IsValidCPPIdentifier(CommittedString))
			{
				if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
				{
					FBlueprintEditorUtils::RenameMemberVariable(Blueprint, IllegalName, FName(CommittedString));
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
