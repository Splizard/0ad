/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "CProgressBar.h"

#include "gui/CGUI.h"

CProgressBar::CProgressBar(CGUI& pGUI)
	: IGUIObject(pGUI),
	  m_SpriteBackground(),
	  m_SpriteBar(),
	  m_Caption()
{
	RegisterSetting("sprite_background", m_SpriteBackground);
	RegisterSetting("sprite_bar", m_SpriteBar);
	RegisterSetting("caption", m_Caption); // aka value from 0 to 100
}

CProgressBar::~CProgressBar()
{
}

void CProgressBar::HandleMessage(SGUIMessage& Message)
{
	// Important
	IGUIObject::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (Message.value == "caption")
		{
			if (m_Caption > 100.f)
				SetSetting<float>("caption", 100.f, true);
			else if (m_Caption < 0.f)
				SetSetting<float>("caption", 0.f, true);
		}
		break;
	default:
		break;
	}
}

void CProgressBar::Draw()
{
	float bz = GetBufferedZ();

	int cell_id = 0;

	m_pGUI.DrawSprite(m_SpriteBackground, cell_id, bz, m_CachedActualSize);

	// Get size of bar (notice it is drawn slightly closer, to appear above the background)
	CRect bar_size(m_CachedActualSize.left, m_CachedActualSize.top,
				   m_CachedActualSize.left+m_CachedActualSize.GetWidth()*(m_Caption/100.f), m_CachedActualSize.bottom);
	m_pGUI.DrawSprite(m_SpriteBar, cell_id, bz+0.01f, bar_size);
}
