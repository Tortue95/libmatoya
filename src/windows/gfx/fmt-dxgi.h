// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

static const DXGI_FORMAT FMT_PLANE0[MTY_COLOR_FORMAT_MAX] = {
	[MTY_COLOR_FORMAT_UNKNOWN]    = DXGI_FORMAT_B8G8R8A8_UNORM,
	[MTY_COLOR_FORMAT_BGRA]       = DXGI_FORMAT_B8G8R8A8_UNORM,
	[MTY_COLOR_FORMAT_NV12]       = DXGI_FORMAT_R8_UNORM,
	[MTY_COLOR_FORMAT_I420]       = DXGI_FORMAT_R8_UNORM,
	[MTY_COLOR_FORMAT_I444]       = DXGI_FORMAT_R8_UNORM,
	[MTY_COLOR_FORMAT_NV16]       = DXGI_FORMAT_R8_UNORM,
	[MTY_COLOR_FORMAT_BGR565]     = DXGI_FORMAT_B5G6R5_UNORM,
	[MTY_COLOR_FORMAT_BGRA5551]   = DXGI_FORMAT_B5G5R5A1_UNORM,
	[MTY_COLOR_FORMAT_AYUV]       = DXGI_FORMAT_B8G8R8A8_UNORM,
	[MTY_COLOR_FORMAT_Y410]       = DXGI_FORMAT_R10G10B10A2_UNORM,
	[MTY_COLOR_FORMAT_Y416]       = DXGI_FORMAT_R16G16B16A16_UNORM,
	[MTY_COLOR_FORMAT_P010]       = DXGI_FORMAT_R16_UNORM,
	[MTY_COLOR_FORMAT_P016]       = DXGI_FORMAT_R16_UNORM,
	[MTY_COLOR_FORMAT_I444_10]    = DXGI_FORMAT_R16_UNORM,
	[MTY_COLOR_FORMAT_I444_16]    = DXGI_FORMAT_R16_UNORM,
};

static const DXGI_FORMAT FMT_PLANE1[MTY_COLOR_FORMAT_MAX] = {
	[MTY_COLOR_FORMAT_UNKNOWN]    = DXGI_FORMAT_B8G8R8A8_UNORM,
	[MTY_COLOR_FORMAT_BGRA]       = DXGI_FORMAT_B8G8R8A8_UNORM,
	[MTY_COLOR_FORMAT_NV12]       = DXGI_FORMAT_R8G8_UNORM,
	[MTY_COLOR_FORMAT_I420]       = DXGI_FORMAT_R8_UNORM,
	[MTY_COLOR_FORMAT_I444]       = DXGI_FORMAT_R8_UNORM,
	[MTY_COLOR_FORMAT_NV16]       = DXGI_FORMAT_R8G8_UNORM,
	[MTY_COLOR_FORMAT_BGR565]     = DXGI_FORMAT_B5G6R5_UNORM,
	[MTY_COLOR_FORMAT_BGRA5551]   = DXGI_FORMAT_B5G5R5A1_UNORM,
	[MTY_COLOR_FORMAT_AYUV]       = DXGI_FORMAT_B8G8R8A8_UNORM,
	[MTY_COLOR_FORMAT_Y410]       = DXGI_FORMAT_R10G10B10A2_UNORM,
	[MTY_COLOR_FORMAT_Y416]       = DXGI_FORMAT_R16G16B16A16_UNORM,
	[MTY_COLOR_FORMAT_P010]       = DXGI_FORMAT_R16G16_UNORM,
	[MTY_COLOR_FORMAT_P016]       = DXGI_FORMAT_R16G16_UNORM,
	[MTY_COLOR_FORMAT_I444_10]    = DXGI_FORMAT_R16_UNORM,
	[MTY_COLOR_FORMAT_I444_16]    = DXGI_FORMAT_R16_UNORM,
};
