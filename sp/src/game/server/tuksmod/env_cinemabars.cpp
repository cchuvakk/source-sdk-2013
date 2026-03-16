#include "cbase.h"
#include "util.h"
#include "player.h"

// Server-side entity to control cinematic bars on clients.
// Hammer classname: env_cinemabars

class CEnvCinemaBars : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvCinemaBars, CPointEntity );
	DECLARE_DATADESC();

	CEnvCinemaBars() : m_iHeight(100), m_bTop(true), m_bBottom(true), m_bStartEnabled(false), m_bVisible(false), m_alpha(255), m_bCoverHud(false), m_flDuration(0.0f), m_flAnimIn(0.35f), m_flAnimOut(0.35f)
	{
		m_color.r = m_color.g = m_color.b = 0;
		m_color.a = 255;
	}

	virtual void Spawn() override
	{
		SetSolid( SOLID_NONE );
		SetMoveType( MOVETYPE_NONE );

		if ( m_bStartEnabled )
		{
			// Show on spawn and mark visible
			BroadcastShow( true );
		}
	}

	void InputShow( inputdata_t &inputdata )
	{
		BroadcastShow( true );
	}

	void InputHide( inputdata_t &inputdata )
	{
		BroadcastShow( false );
	}

	void InputToggle( inputdata_t &inputdata )
	{
		// Toggle visibility based on current state
		BroadcastShow( !m_bVisible );
	}

	void InputSetHeight( inputdata_t &inputdata )
	{
		int newH = (int)inputdata.value.Int();
		if ( newH < 0 )
			newH = 0;
		m_iHeight = newH;

		// If currently visible, update clients with new height
		if ( m_bVisible )
		{
			BroadcastShow( true );
		}
	}

	void InputSetTop( inputdata_t &inputdata )
	{
		m_bTop = inputdata.value.Bool();

		// If currently visible, update clients with new mode
		if ( m_bVisible )
		{
			BroadcastShow( true );
		}
	}

	void InputSetBottom( inputdata_t &inputdata )
	{
		m_bBottom = inputdata.value.Bool();

		// If currently visible, update clients with new mode
		if ( m_bVisible )
		{
			BroadcastShow( true );
		}
	}

	void InputSetColor( inputdata_t &inputdata )
	{
		const char *s = inputdata.value.String();
		if ( s && s[0] )
		{
			int r = 0, g = 0, b = 0;
			sscanf( s, "%d %d %d", &r, &g, &b );
			m_color.r = clamp(r, 0, 255);
			m_color.g = clamp(g, 0, 255);
			m_color.b = clamp(b, 0, 255);
		}

		if ( m_bVisible )
			BroadcastShow( true );
	}

	void InputSetAlpha( inputdata_t &inputdata )
	{
		int a = (int)inputdata.value.Int();
		m_alpha = clamp(a, 0, 255);
		if ( m_bVisible )
			BroadcastShow( true );
	}

	void InputSetCover( inputdata_t &inputdata )
	{
		m_bCoverHud = inputdata.value.Bool();
		if ( m_bVisible )
			BroadcastShow( true );
	}

	void InputSetDuration( inputdata_t &inputdata )
	{
		m_flDuration = inputdata.value.Float();
		if ( m_flDuration < 0.0f )
			m_flDuration = 0.0f;
		if ( m_bVisible )
			BroadcastShow( true );
	}

	void InputSetAnimIn( inputdata_t &inputdata )
	{
		m_flAnimIn = inputdata.value.Float();
		if ( m_flAnimIn < 0.0f ) m_flAnimIn = 0.0f;
		if ( m_bVisible ) BroadcastShow( true );
	}

	void InputSetAnimOut( inputdata_t &inputdata )
	{
		m_flAnimOut = inputdata.value.Float();
		if ( m_flAnimOut < 0.0f ) m_flAnimOut = 0.0f;
		if ( m_bVisible ) BroadcastShow( true );
	}

private:
	void BroadcastShow( bool bShow )
	{
		m_bVisible = bShow;

		const char *mode = "both";
		if ( m_bTop && !m_bBottom ) mode = "top";
		else if ( m_bBottom && !m_bTop ) mode = "bottom";

		if ( gpGlobals->maxClients <= 1 )
		{
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				if ( bShow )
					engine->ClientCommand( pPlayer->edict(), "show_cinemabars 1 %d %s %d %d %d %d %d %f %f %f\n", m_iHeight, mode, m_color.r, m_color.g, m_color.b, m_alpha, m_bCoverHud ? 1 : 0, m_flDuration, m_flAnimIn, m_flAnimOut );
				else
					engine->ClientCommand( pPlayer->edict(), "show_cinemabars 0\n" );
			}
			return;
		}

		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			if ( bShow )
				engine->ClientCommand( pPlayer->edict(), "show_cinemabars 1 %d %s %d %d %d %d %d %f %f %f\n", m_iHeight, mode, m_color.r, m_color.g, m_color.b, m_alpha, m_bCoverHud ? 1 : 0, m_flDuration, m_flAnimIn, m_flAnimOut );
			else
				engine->ClientCommand( pPlayer->edict(), "show_cinemabars 0\n" );
		}
	}

	void BroadcastToggle()
	{
		// Deprecated: use BroadcastShow(!m_bVisible) instead
		BroadcastShow( !m_bVisible );
	}

	int m_iHeight;
	bool m_bTop;
	bool m_bBottom;
	bool m_bStartEnabled;
	bool m_bVisible;

	color32 m_color;
	int m_alpha;
	bool m_bCoverHud;
	float m_flDuration;
	float m_flAnimIn;
	float m_flAnimOut;
};

LINK_ENTITY_TO_CLASS( env_cinemabars, CEnvCinemaBars );

BEGIN_DATADESC( CEnvCinemaBars )

	DEFINE_KEYFIELD( m_iHeight, FIELD_INTEGER, "height" ),
	DEFINE_KEYFIELD( m_bTop, FIELD_BOOLEAN, "top" ),
	DEFINE_KEYFIELD( m_bBottom, FIELD_BOOLEAN, "bottom" ),
	DEFINE_KEYFIELD( m_bStartEnabled, FIELD_BOOLEAN, "startenabled" ),
	DEFINE_KEYFIELD( m_color, FIELD_COLOR32, "color" ),
	DEFINE_KEYFIELD( m_alpha, FIELD_INTEGER, "alpha" ),
	DEFINE_KEYFIELD( m_bCoverHud, FIELD_BOOLEAN, "coverhud" ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_flAnimIn, FIELD_FLOAT, "anim_in" ),
	DEFINE_KEYFIELD( m_flAnimOut, FIELD_FLOAT, "anim_out" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Show", InputShow ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHeight", InputSetHeight ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetTop", InputSetTop ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetBottom", InputSetBottom ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetColor", InputSetColor ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAlpha", InputSetAlpha ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCover", InputSetCover ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDuration", InputSetDuration ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAnimIn", InputSetAnimIn ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAnimOut", InputSetAnimOut ),

END_DATADESC()
