#pragma once

#include <cbase.h>
#include "c_vguiscreen.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ToggleButton.h"

class AppartmentPowerDisablePanel : public CVGuiScreenPanel
{
private:
	DECLARE_CLASS(AppartmentPowerDisablePanel, CVGuiScreen);

	vgui::Label* m_pLblWelcome;

	vgui::ToggleButton* m_pBtnApartment[9];

public:
	AppartmentPowerDisablePanel(vgui::Panel* pParent, const char* pMetaClassName);
	virtual ~AppartmentPowerDisablePanel();

	virtual void OnCommand(const char* command);
	virtual bool Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData);
};