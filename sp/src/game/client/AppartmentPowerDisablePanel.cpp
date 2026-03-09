#include <cbase.h>
#include "AppartmentPowerDisablePanel.h"
#include "vgui/IVGui.h"


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

		m_pBtnApartment[i] = dynamic_cast<vgui::Button*>(FindChildByName(name));

		if (m_pBtnApartment[i])
		{
			m_pBtnApartment[i]->SetPaintBackgroundEnabled(true);
			//dunno if it's necessary
			m_pBtnApartment[i]->SetBgColor(Color(170, 170, 170, 255));
			m_pBtnApartment[i]->SetFgColor(Color(0, 0, 0, 255));
			// ^^^ this one
			m_pBtnApartment[i]->SetDepressedColor(Color(41, 49, 51, 255), Color(255, 255, 255, 255));
			m_pBtnApartment[i]->SetDepressedSound("buttons/button14.wav");
		}
	}

	return true;
}


void AppartmentPowerDisablePanel::OnCommand(const char* command)
{
	CVGuiScreenPanel::OnCommand(command);

	for (int i = 0; i < 9; i++) 
	{
		char name[32];
		sprintf(name, "apartment%02d", i + 1);

		if (!Q_stricmp(command, name))
		{
			if (m_pBtnApartment[i])
			{
				m_pBtnApartment[i]->SetEnabled(false);
			}
		}
	}
}


void AppartmentPowerDisablePanel::PerformLayout()
{
	CVGuiScreenPanel::PerformLayout();

	for (int i = 0; i < 9; i++)
	{
		if (m_pBtnApartment[i]) 
		{
			m_pBtnApartment[i]->SetDefaultColor(Color(0, 0, 0, 255), Color(170, 170, 170, 255));
			m_pBtnApartment[i]->SetDisabledFgColor1(Color(50, 50, 50, 255));
			m_pBtnApartment[i]->SetDisabledFgColor2(Color(0, 0, 0, 255));
		}
	}
}

DECLARE_VGUI_SCREEN_FACTORY(AppartmentPowerDisablePanel, "AppartmentPowerDisable");