#include "UI/OutbreakMenuButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

UOutbreakMenuButton* UOutbreakMenuButton::Create(UWidgetTree& Tree, const FText& Label, int32 InAction, int32 InIndex)
{
	UOutbreakMenuButton* Button = Tree.ConstructWidget<UOutbreakMenuButton>(UOutbreakMenuButton::StaticClass());
	Button->Action = InAction;
	Button->Index = InIndex;
	Button->OnClicked.AddUniqueDynamic(Button, &UOutbreakMenuButton::HandleClicked);

	Button->LabelText = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Button->LabelText->SetText(Label);
	Button->LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	Button->AddChild(Button->LabelText);
	return Button;
}

void UOutbreakMenuButton::SetLabel(const FText& Label)
{
	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

void UOutbreakMenuButton::HandleClicked()
{
	OnMenuButtonClicked.Broadcast(this);
}
