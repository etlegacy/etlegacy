/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file iqm.h
 */

#ifndef INCLUDE_IQM_H
#define INCLUDE_IQM_H

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2

#define IQM_MAX_JOINTS      128

/**
 * @struct iqmheader
 * @typedef iqmHeader_t
 * @brief
 */
typedef struct iqmheader
{
	char magic[16];
	unsigned int version;
	unsigned int filesize;
	unsigned int flags;
	unsigned int num_text, ofs_text;
	unsigned int num_meshes, ofs_meshes;
	unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
	unsigned int num_triangles, ofs_triangles, ofs_adjacency;
	unsigned int num_joints, ofs_joints;
	unsigned int num_poses, ofs_poses;
	unsigned int num_anims, ofs_anims;
	unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
	unsigned int num_comment, ofs_comment;
	unsigned int num_extensions, ofs_extensions;
} iqmHeader_t;

/**
 * @struct iqmmesh
 * @typedef iqmMesh_t
 * @brief
 */
typedef struct iqmmesh
{
	unsigned int name;
	unsigned int material;
	unsigned int first_vertex, num_vertexes;
	unsigned int first_triangle, num_triangles;
} iqmMesh_t;

enum
{
	IQM_POSITION     = 0,
	IQM_TEXCOORD     = 1,
	IQM_NORMAL       = 2,
	IQM_TANGENT      = 3,
	IQM_BLENDINDEXES = 4,
	IQM_BLENDWEIGHTS = 5,
	IQM_COLOR        = 6,
	IQM_CUSTOM       = 0x10
};

enum
{
	IQM_BYTE   = 0,
	IQM_UBYTE  = 1,
	IQM_SHORT  = 2,
	IQM_USHORT = 3,
	IQM_INT    = 4,
	IQM_UINT   = 5,
	IQM_HALF   = 6,
	IQM_FLOAT  = 7,
	IQM_DOUBLE = 8
};

/**
 * @struct iqmtriangle
 * @typedef iqmTriangle_t
 * @brief
 */
typedef struct iqmtriangle
{
	unsigned int vertex[3];
} iqmTriangle_t;


/**
 * @struct iqmjointv1
 * @typedef iqmJointv1_t
 * @brief
 */
typedef struct iqmjointv1
{
	unsigned int name;
	int parent;
	float translate[3], rotate[3], scale[3];
} iqmJointv1_t;

/**
 * @struct iqmjoint
 * @typedef iqmJoint_t
 * @brief
 */
typedef struct iqmjoint
{
	unsigned int name;
	int parent;
	float translate[3], rotate[4], scale[3];
} iqmJoint_t;

/**
 * @struct iqmposev1
 * @typedef iqmPosev1_t
 * @brief
 */
typedef struct iqmposev1
{
	int parent;
	unsigned int mask;
	float channeloffset[9];
	float channelscale[9];
} iqmPosev1_t;

/**
 * @struct iqmpose
 * @typedef iqmPose_t
 * @brief
 */
typedef struct iqmpose
{
	int parent;
	unsigned int mask;
	float channeloffset[10];
	float channelscale[10];
} iqmPose_t;

/**
 * @struct iqmanim
 * @typedef iqmAnim_t
 * @brief
 */
typedef struct iqmanim
{
	unsigned int name;
	unsigned int first_frame, num_frames;
	float framerate;
	unsigned int flags;
} iqmAnim_t;

enum
{
	IQM_LOOP = 1 << 0
};

/**
 * @struct iqmvertexarray
 * @typedef iqmVertexArray_t
 * @brief
 */
typedef struct iqmvertexarray
{
	unsigned int type;
	unsigned int flags;
	unsigned int format;
	unsigned int size;
	unsigned int offset;
} iqmVertexArray_t;

/**
 * @struct iqmbounds
 * @typedef iqmBounds_t
 * @brief
 */
typedef struct iqmbounds
{
	float bbmin[3], bbmax[3];
	float xyradius, radius;
} iqmBounds_t;

/**
 * @struct iqmextension
 * @typedef iqmExtension_t
 * @brief
 */
typedef struct iqmextension
{
	unsigned int name;
	unsigned int num_data, ofs_data;
	unsigned int ofs_extensions; // pointer to next extension
} iqmExtension_t;

#endif // INCLUDE_IQM_H
