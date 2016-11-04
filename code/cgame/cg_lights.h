/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2016 Witold Pilat <witold.pilat@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
================================================================================
*/

#pragma once
#if !defined(CG_LIGHTS_H_INC)
#define CG_LIGHTS_H_INC

typedef struct
{
	int				length;
	color4ub_t		value;
	color4ub_t		map[MAX_QPATH];
} clightstyle_t;

void	CG_ClearLightStyles (void);
void	CG_RunLightStyles (void);
void    CG_SetLightstyle (int i);

#endif // CG_LIGHTS_H_INC
