#include <cbase.h>
#include "AppartmentPowerDisablePanel.h"
#include "vgui/IVGui.h"

#include <sstream>

AppartmentPowerDisablePanel::AppartmentPowerDisablePanel(vgui::Panel* pParent, const char* pMetaClassName) : CVGuiScreenPanel(pParent, pMetaClassName)
{
	m_pLblWelcome = 0;

	for (int i = 0; i < 9; i++)
	{
		m_pBtnApartment[i] = 0;
	}
}

AppartmentPowerDisablePanel::~AppartmentPowerDisablePanel()
{
}

bool AppartmentPowerDisablePanel::Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData)
{
	if (!CVGuiScreenPanel::Init(pKeyValues, pInitData))
	{
		return false;
	}

	m_pLblWelcome = dynamic_cast<vgui::Label*>(FindChildByName("lblWelcome"));

	for (int i = 0; i < 9; i++)
	{
		char name[32];
		sprintf(name, "Apartment%02d", i + 1);

		m_pBtnApartment[i] = dynamic_cast<vgui::ToggleButton*>(FindChildByName(name));
		m_pBtnApartment[i]->SetDepressedSound("ambient/machines/thumper_startup1.wav");
		m_pBtnApartment[i]->SetReleasedSound("ambient/machines/thumper_shutdown1.wav");
	}

	return true;
}

void AppartmentPowerDisablePanel::OnCommand(const char* command)
{
	for (int i = 0; i < 9; i++)
	{
		if (m_pBtnApartment[i]->IsSelected())
		{

		}
	}
}

DECLARE_VGUI_SCREEN_FACTORY(AppartmentPowerDisablePanel, "AppartmentPowerDisable");