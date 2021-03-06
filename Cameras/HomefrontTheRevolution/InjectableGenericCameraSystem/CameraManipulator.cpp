////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2017, Frans Bouma
// All rights reserved.
// https://github.com/FransBouma/InjectableGenericCameraSystem
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CameraManipulator.h"
#include "GameConstants.h"
#include "InterceptorHelper.h"
#include "Globals.h"
#include "OverlayConsole.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_timestopStructAddress = nullptr;
	LPBYTE g_fovStructAddress = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalMatrixData[12];
	static float _currentCameraCoords[3];
	static float _originalFov;

	void setSupersamplingFactor(LPBYTE hostImageAddress, float newValue)
	{
		float* superSamplingVarAddress = reinterpret_cast<float*>(hostImageAddress + SUPERSAMPLING_IN_IMAGE_OFFSET);
		*superSamplingVarAddress = newValue;
	}


	// newValue: 1 == time should be frozen, 0 == normal gameplay
	void setTimeStopValue(byte newValue)
	{
		if (nullptr == g_timestopStructAddress)
		{
			return;
		}
		*(g_timestopStructAddress + TIMESTOP_IN_STRUCT_OFFSET) = newValue;
	}

	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV()
	{
		if (nullptr == g_fovStructAddress)
		{
			return;
		}
		float* fovInMemory = reinterpret_cast<float*>(g_fovStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory = _originalFov;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (nullptr == g_fovStructAddress)
		{
			return;
		}
		float* fovInMemory = reinterpret_cast<float*>(g_fovStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory += amount;
	}


	XMFLOAT3 getCurrentCameraCoords()
	{
		return XMFLOAT3(_currentCameraCoords[0], _currentCameraCoords[1], _currentCameraCoords[2]);
	}
	

	// newLookQuaternion: newly calculated quaternion of camera view space. Can be used to construct a 4x4 matrix if the game uses a matrix instead of a quaternion
	// newCoords are the new coordinates for the camera in worldspace.
	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		// game uses a 3x3 matrix for look data. We have to calculate a rotation matrix from our quaternion and store the upper 3x3 matrix (_11-_33) in memory.
		XMMATRIX rotationMatrixPacked = XMMatrixRotationQuaternion(newLookQuaternion);
		XMFLOAT4X4 rotationMatrix;
		XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress+MATRIX_IN_STRUCT_OFFSET);
		matrixInMemory[0] = rotationMatrix._11;
		matrixInMemory[1] = rotationMatrix._21;
		matrixInMemory[2] = rotationMatrix._31;
		matrixInMemory[3] = newCoords.x;
		matrixInMemory[4] = rotationMatrix._12;
		matrixInMemory[5] = rotationMatrix._22;
		matrixInMemory[6] = rotationMatrix._32;
		matrixInMemory[7] = newCoords.y;
		matrixInMemory[8] = rotationMatrix._13;
		matrixInMemory[9] = rotationMatrix._23;
		matrixInMemory[10] = rotationMatrix._33;
		matrixInMemory[11] = newCoords.z;

		_currentCameraCoords[0] = newCoords.x;
		_currentCameraCoords[1] = newCoords.y;
		_currentCameraCoords[2] = newCoords.z;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalCameraValues()
	{
		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress+ MATRIX_IN_STRUCT_OFFSET);
		memcpy(matrixInMemory, _originalMatrixData, 12 * sizeof(float));
		float* fovInMemory = reinterpret_cast<float*>(g_fovStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory = _originalFov;
	}


	void cacheOriginalCameraValues()
	{
		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
		memcpy(_originalMatrixData, matrixInMemory, 12 * sizeof(float));
		_currentCameraCoords[0] = _originalMatrixData[3];		// x
		_currentCameraCoords[1] = _originalMatrixData[7];		// y
		_currentCameraCoords[2] = _originalMatrixData[11];		// z
		float* fovInMemory = reinterpret_cast<float*>(g_fovStructAddress + FOV_IN_STRUCT_OFFSET);
		_originalFov = *fovInMemory;
	}
}