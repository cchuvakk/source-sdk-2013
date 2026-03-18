#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include "hud.h"
#include "clientmode.h"
#include "usermessages.h"

using namespace vgui;

#define CINEMABARS_ANIM_TICK_MS 10

// Simple cinematic bars HUD panel — full-width, anchored to top or bottom, configurable color/alpha and z-order.
class CHudCinemaBars : public Panel
{
	DECLARE_CLASS_SIMPLE(CHudCinemaBars, Panel);

public:
	CHudCinemaBars(vgui::VPANEL parent, bool bTop = false, int iHeight = 100, int r = 0, int g = 0, int b = 0, int a = 255, bool bCoverHud = false, float flDuration = 0.0f, float flAnimIn = 0.35f, float flAnimOut = 0.35f);
	~CHudCinemaBars() override { vgui::ivgui()->RemoveTickSignal(GetVPanel()); }

	virtual void PerformLayout() override;
	virtual void Paint() override;
	virtual void OnTick() override;

	void StartSlideOut();
	void UpdateParams(int iHeight, int r, int g, int b, int a, bool bCoverHud, float flDuration, float flAnimIn, float flAnimOut);

private:
	int m_iHeight; // base height in pixels (interpreted at 1080p and scaled)
	bool m_bTop; // anchor to top if true, bottom if false
	int m_r, m_g, m_b, m_a;

	// Animation
	enum AnimPhase { ANIM_NONE = 0, ANIM_SLIDE_IN, ANIM_SLIDE_OUT };
	AnimPhase m_eAnimPhase;
	float m_flAnimStartTime;
	int m_iAnimStartY;
	int m_iFinalY;
	bool m_bPendingDelete;

	// Auto-hide
	float m_flDuration; // seconds to show after slide-in completes
	float m_flAutoHideAt; // gpGlobals->realtime time when we should start slide-out
	bool m_bAutoHideStarted;

	// server-provided anim durations
	float m_flAnimIn;
	float m_flAnimOut;
};

static CHudCinemaBars* g_pHudCinemaBarsTop = nullptr;
static CHudCinemaBars* g_pHudCinemaBarsBottom = nullptr;

// Forward decl for usermessage handler
static void MsgFunc_CinemaBars(bf_read& msg);

CHudCinemaBars::CHudCinemaBars(vgui::VPANEL parent, bool bTop, int iHeight, int r, int g, int b, int a, bool bCoverHud, float flDuration, float flAnimIn, float flAnimOut)
	: Panel(nullptr, "HudCinemaBars"), m_iHeight(iHeight), m_bTop(bTop), m_r(r), m_g(g), m_b(b), m_a(a),
	m_eAnimPhase(ANIM_NONE), m_flAnimStartTime(0.0f), m_iAnimStartY(0), m_iFinalY(0), m_bPendingDelete(false),
	m_flDuration(flDuration), m_flAutoHideAt(0.0f), m_bAutoHideStarted(false), m_flAnimIn(flAnimIn), m_flAnimOut(flAnimOut)
{
	SetParent(parent);
	// Set ZPos so it either covers HUD or stays behind it.
	// Use a high value to be above most HUD elements when covering.
	const int COVER_ZPOS = 30000;
	const int BACK_ZPOS = -666;
	SetZPos(bCoverHud ? COVER_ZPOS : BACK_ZPOS);
	SetVisible(true);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(false);
	SetBgColor(Color(r, g, b, a));

	// Start slide-in animation
	m_eAnimPhase = ANIM_SLIDE_IN;
	m_flAnimStartTime = gpGlobals->realtime;

	// Receive ticks to animate and handle auto-hide
	vgui::ivgui()->AddTickSignal(GetVPanel(), CINEMABARS_ANIM_TICK_MS);
}

void CHudCinemaBars::UpdateParams(int iHeight, int r, int g, int b, int a, bool bCoverHud, float flDuration, float flAnimIn, float flAnimOut)
{
	m_iHeight = iHeight;
	m_r = r; m_g = g; m_b = b; m_a = a;
	m_flDuration = flDuration;
	m_flAnimIn = flAnimIn;
	m_flAnimOut = flAnimOut;
	SetBgColor(Color(r, g, b, a));
	const int COVER_ZPOS = 30000;
	const int BACK_ZPOS = -666;
	SetZPos(bCoverHud ? COVER_ZPOS : BACK_ZPOS);

	// If this panel was sliding out or pending delete, cancel and start slide-in so update takes effect
	if (m_eAnimPhase == ANIM_SLIDE_OUT || m_bPendingDelete)
	{
		m_bPendingDelete = false;
		m_eAnimPhase = ANIM_SLIDE_IN;
		m_flAnimStartTime = gpGlobals->realtime;
		// ensure ticks
		vgui::ivgui()->AddTickSignal(GetVPanel(), CINEMABARS_ANIM_TICK_MS);
	}
	else if (m_eAnimPhase == ANIM_NONE)
	{
		// restart slide-in to animate any changed values
		m_eAnimPhase = ANIM_SLIDE_IN;
		m_flAnimStartTime = gpGlobals->realtime;
		vgui::ivgui()->AddTickSignal(GetVPanel(), CINEMABARS_ANIM_TICK_MS);
	}
	InvalidateLayout();
}

void CHudCinemaBars::OnTick()
{
	// If we are pending deletion and animation finished, delete safely here BEFORE any base OnTick calls
	if (m_bPendingDelete && m_eAnimPhase == ANIM_NONE)
	{
		vgui::ivgui()->RemoveTickSignal(GetVPanel());
		if (this == g_pHudCinemaBarsTop) g_pHudCinemaBarsTop = nullptr;
		if (this == g_pHudCinemaBarsBottom) g_pHudCinemaBarsBottom = nullptr;
		SetParent((vgui::Panel*)NULL);
		delete this;
		return; // deleted
	}

	BaseClass::OnTick();

	// Auto-hide: if slide-in completed and duration specified, hide after duration elapses
	if (m_bAutoHideStarted && m_flAutoHideAt > 0.0f && m_eAnimPhase == ANIM_NONE && !m_bPendingDelete)
	{
		if (gpGlobals->realtime >= m_flAutoHideAt)
		{
			StartSlideOut();
			return; // slide-out started, will handle deletion
		}
	}

	// Invalidate layout so PerformLayout can compute interpolated position
	InvalidateLayout();

	// If anim ended and not pending delete and no auto-hide pending, stop ticks
	if (m_eAnimPhase == ANIM_NONE && !m_bPendingDelete && !(m_bAutoHideStarted && m_flAutoHideAt > gpGlobals->realtime))
	{
		vgui::ivgui()->RemoveTickSignal(GetVPanel());
	}
}

void CHudCinemaBars::PerformLayout()
{
	int sw, sh;
	vgui::surface()->GetScreenSize(sw, sh);

	// Scale the configured base height (assumed for 1080p) so bars look consistent across resolutions.
	const float BASE_HEIGHT_REF = 1080.0f; // baseline reference height
	int scaledH = MAX(1, (int)floorf(m_iHeight * ((float)sh / BASE_HEIGHT_REF) + 0.5f));

	// Compute final resting Y
	m_iFinalY = m_bTop ? 0 : (sh - scaledH);

	int panelW = sw;
	int panelH = scaledH;
	int panelX = 0;
	int panelY = m_iFinalY;

	if (m_eAnimPhase == ANIM_SLIDE_IN)
	{
		float flElapsed = gpGlobals->realtime - m_flAnimStartTime;
		float flDuration = (m_flAnimIn > 0.0f) ? m_flAnimIn : 0.0001f;
		float flFraction = clamp(flElapsed / flDuration, 0.0f, 1.0f);
		// Ease-out
		float flSmooth = 1.0f - (1.0f - flFraction) * (1.0f - flFraction);

		int startY = m_bTop ? -panelH : sh;
		panelY = startY + (int)((float)(m_iFinalY - startY) * flSmooth);

		if (flFraction >= 1.0f)
		{
			m_eAnimPhase = ANIM_NONE;
			panelY = m_iFinalY;
			// Start auto-hide countdown now that slide-in finished
			if (m_flDuration > 0.0f && !m_bAutoHideStarted)
			{
				m_bAutoHideStarted = true;
				m_flAutoHideAt = gpGlobals->realtime + m_flDuration;
				// ensure ticks stay active until auto-hide triggers
				vgui::ivgui()->AddTickSignal(GetVPanel(), CINEMABARS_ANIM_TICK_MS);
			}
		}
	}
	else if (m_eAnimPhase == ANIM_SLIDE_OUT)
	{
		float flElapsed = gpGlobals->realtime - m_flAnimStartTime;
		float flDuration = (m_flAnimOut > 0.0f) ? m_flAnimOut : 0.0001f;
		float flFraction = clamp(flElapsed / flDuration, 0.0f, 1.0f);
		// Ease-in
		float flSmooth = flFraction * flFraction;

		int targetY = m_bTop ? -panelH : sh;
		int startY = m_iAnimStartY;
		panelY = startY + (int)((float)(targetY - startY) * flSmooth);

		if (flFraction >= 1.0f)
		{
			// Mark pending delete; actual delete will occur in OnTick
			m_eAnimPhase = ANIM_NONE;
			m_bPendingDelete = true;
			// keep ticks so OnTick will delete
			return;
		}
	}

	SetBounds(panelX, panelY, panelW, panelH);
}

void CHudCinemaBars::Paint()
{
	int w, h;
	GetSize(w, h);

	// Draw rectangle covering the whole panel area with configured color and alpha
	vgui::surface()->DrawSetColor(m_r, m_g, m_b, m_a);
	vgui::surface()->DrawFilledRect(0, 0, w, h);
}

void CHudCinemaBars::StartSlideOut()
{
	if (m_eAnimPhase == ANIM_SLIDE_OUT || m_bPendingDelete)
		return;

	int curX, curY;
	GetPos(curX, curY);
	m_iAnimStartY = curY;
	m_eAnimPhase = ANIM_SLIDE_OUT;
	m_flAnimStartTime = gpGlobals->realtime;
	// Ensure we're receiving ticks until animation finishes
	vgui::ivgui()->AddTickSignal(GetVPanel(), CINEMABARS_ANIM_TICK_MS);
}

// Helper create/destroy functions for other code to use
void CreateHudCinemaBars(vgui::VPANEL parent, int iHeight = 100, const char* mode = "both", int r = 0, int g = 0, int b = 0, int a = 255, bool bCoverHud = false, float flDuration = 0.0f, float flAnimIn = 0.35f, float flAnimOut = 0.35f)
{
	// If existing panels, update them instead of creating duplicates
	if (!Q_stricmp(mode, "top"))
	{
		if (g_pHudCinemaBarsTop)
		{
			g_pHudCinemaBarsTop->UpdateParams(iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
		else
		{
			g_pHudCinemaBarsTop = new CHudCinemaBars(parent, true, iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
	}
	else if (!Q_stricmp(mode, "bottom"))
	{
		if (g_pHudCinemaBarsBottom)
		{
			g_pHudCinemaBarsBottom->UpdateParams(iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
		else
		{
			g_pHudCinemaBarsBottom = new CHudCinemaBars(parent, false, iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
	}
	else // both
	{
		if (g_pHudCinemaBarsTop)
		{
			g_pHudCinemaBarsTop->UpdateParams(iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
		else
		{
			g_pHudCinemaBarsTop = new CHudCinemaBars(parent, true, iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}

		if (g_pHudCinemaBarsBottom)
		{
			g_pHudCinemaBarsBottom->UpdateParams(iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
		else
		{
			g_pHudCinemaBarsBottom = new CHudCinemaBars(parent, false, iHeight, r, g, b, a, bCoverHud, flDuration, flAnimIn, flAnimOut);
		}
	}
}

void DestroyHudCinemaBars()
{
	if (g_pHudCinemaBarsTop)
	{
		g_pHudCinemaBarsTop->StartSlideOut();
		g_pHudCinemaBarsTop = nullptr;
	}
	if (g_pHudCinemaBarsBottom)
	{
		g_pHudCinemaBarsBottom->StartSlideOut();
		g_pHudCinemaBarsBottom = nullptr;
	}
}

CHudCinemaBars* GetHudCinemaBarsTop()
{
	return g_pHudCinemaBarsTop;
}

CHudCinemaBars* GetHudCinemaBarsBottom()
{
	return g_pHudCinemaBarsBottom;
}

// Usermessage handler for server-controlled cinematic bars
static void MsgFunc_CinemaBars(bf_read& msg)
{
	int bShow = msg.ReadByte();
	if (bShow)
	{
		int height = msg.ReadShort();
		int modeFlags = msg.ReadByte();
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int alpha = msg.ReadByte();
		int cover = msg.ReadByte();
		float duration = msg.ReadFloat();
		float animIn = msg.ReadFloat();
		float animOut = msg.ReadFloat();

		const char* mode = "both";
		if (modeFlags == 1) mode = "top";
		else if (modeFlags == 2) mode = "bottom";

		vgui::VPANEL parent = NULL;
		if (g_pClientMode && g_pClientMode->GetViewport())
			parent = g_pClientMode->GetViewport()->GetVPanel();
		if (!parent)
			return;

		CreateHudCinemaBars(parent, height, mode, r, g, b, alpha, cover != 0, duration, animIn, animOut);
	}
	else
	{
		// hide
		if (GetHudCinemaBarsTop() || GetHudCinemaBarsBottom())
			DestroyHudCinemaBars();
	}
}

// Hook message at static init time
struct CCinemaBarsMsgRegistrar
{
	CCinemaBarsMsgRegistrar()
	{
		if (usermessages)
		{
			usermessages->HookMessage("CinemaBars", MsgFunc_CinemaBars);
		}
	}
};

static CCinemaBarsMsgRegistrar g_CinemaBarsMsgRegistrar;