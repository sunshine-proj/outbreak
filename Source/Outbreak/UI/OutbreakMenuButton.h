#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"

#include "OutbreakMenuButton.generated.h"

class UOutbreakMenuButton;
class UTextBlock;
class UWidgetTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakMenuButtonClickedSignature, UOutbreakMenuButton*, Button);

/**
 * A text button that reports which button was clicked, with an action and an index, so a screen
 * built in C++ can handle a whole list of buttons with one handler (UButton::OnClicked has no
 * parameters).
 */
UCLASS()
class OUTBREAK_API UOutbreakMenuButton : public UButton
{
	GENERATED_BODY()

public:

	/** Creates a button labelled Label in Tree. */
	static UOutbreakMenuButton* Create(UWidgetTree& Tree, const FText& Label, int32 InAction, int32 InIndex = 0);

	void SetLabel(const FText& Label);

	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FOutbreakMenuButtonClickedSignature OnMenuButtonClicked;

	/** What the button does, interpreted by the screen that created it. */
	int32 Action = 0;

	/** Which row or member it belongs to. */
	int32 Index = 0;

private:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;
};
