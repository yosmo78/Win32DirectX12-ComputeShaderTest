#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//windows
#include <windows.h>

#if !_M_X64
#error 64-BIT platform required!
#endif

//direct x
#include <d3d12.h>
#include <dxgi1_4.h> 

#if MAIN_DEBUG
#include <dxgidebug.h>
#	if RUNTIME_DEBUG_COMPILE
#	include <D3Dcompiler.h> 
#	else
#		if !COMPILED_DEBUG_CSO
#		include "computeShaderDebug.h"
#		endif
#	endif
#else
#include "computeShader.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#define PI_F 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679f
#define PI_D 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float    f32; //floating 32
typedef double   f64; //floating 64


typedef struct Mat3f
{
	union
	{
		f32 m[3][3];
	};
} Mat3f;

typedef struct Mat4f
{
	union
	{
		f32 m[4][4];
	};
} Mat4f;

typedef struct Mat3x4f
{
	union
	{
		f32 m[3][4];
	};
} Mat3x4f;

typedef struct Vec2f
{
	union
	{
		f32 v[2];
		struct
		{
			f32 x;
			f32 y;
		};
	};
} Vec2f;

typedef struct Vec3f
{
	union
	{
		f32 v[3];
		struct
		{
			f32 x;
			f32 y;
			f32 z;
		};
	};
} Vec3f;

typedef struct Vec4f
{
	union
	{
		f32 v[4];
		struct
		{
			f32 x;
			f32 y;
			f32 z;
			f32 w;
		};
	};
} Vec4f;

typedef struct Quatf
{
	union
	{
		f32 q[4];
		struct
		{
			f32 w; //real
			f32 x;
			f32 y;
			f32 z;
		};
		struct
		{
			f32 real; //real;
			Vec3f v;
		};
	};
} Quatf;

//Amazing page https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization?redirectedfrom=MSDN
ID3D12Device2* device;
ID3D12CommandQueue* computeQueue;
ID3D12CommandAllocator* computeCommandAllocator[2];
ID3D12GraphicsCommandList* computeCommandList;
ID3D12CommandQueue* streamingQueue;
ID3D12CommandAllocator* streamingCommandAllocator[2];
ID3D12GraphicsCommandList* streamingCommandList;
UINT cbvsrvuavDescriptorSize;

//GPU/CPU syncronization
ID3D12Fence* computeFence;
u64 computeFenceValue;
HANDLE computeFenceEvent;
ID3D12Fence* streamingFence;
u64 streamingFenceValue;
HANDLE streamingFenceEvent;

//All views
//Remeber views are required so the gpu can see and understand a resourse (only exception is root constants), so if anything is to be used by a shader it must have a view
//(for any I/O needs a view, backbuffers(swapchain and depth and stencil buffer), uniforms, vertex/index inputs, textures)
D3D12_VERTEX_BUFFER_VIEW planeVertexBufferView;
D3D12_INDEX_BUFFER_VIEW planeIndexBufferView;
u32 planeIndexCount;
D3D12_VERTEX_BUFFER_VIEW cubeVertexBufferView;
D3D12_INDEX_BUFFER_VIEW cubeIndexBufferView;
u32 cubeIndexCount;

ID3D12Heap* pModelDefaultHeap;
ID3D12Heap* pModelUploadHeap; //will be <= size of the default heap
ID3D12Heap* pComputeOutputHeap;
ID3D12Heap* pReadbackHeap;

ID3D12Resource* defaultBuffer; //a default placed resource
ID3D12Resource* uploadBuffer; //a tmp upload placed resource
ID3D12Resource* computeOutputBuffer[2]; //a readback placed resource
ID3D12Resource* readbackBuffer[2]; //a readback placed resource

//pipeline info
ID3D12RootSignature* computeRootSignature; // root signature defines data shaders will access
ID3D12PipelineState* computePipelineStateObject; // pso containing a pipeline state

#if MAIN_DEBUG
ID3D12Debug *debugInterface;
ID3D12InfoQueue *pIQueue; 
#endif

typedef struct ComputeShaderCB
{
	u32 dwOffsetsAndStrides0[4];
} ComputeShaderCB;

typedef struct ModelOutData
{
	u32 dwData[4];
} ModelOutData;

inline
void InitMat3f( Mat3f *a_pMat )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = 0; a_pMat->m[2][2] = 1;
}

inline
void InitMat4f( Mat4f *a_pMat )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0; a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0; a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = 0; a_pMat->m[2][2] = 1; a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0; a_pMat->m[3][1] = 0; a_pMat->m[3][2] = 0; a_pMat->m[3][3] = 1;
}

inline
void InitTransMat4f( Mat4f *a_pMat, f32 x, f32 y, f32 z )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0; a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0; a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = 0; a_pMat->m[2][2] = 1; a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = x; a_pMat->m[3][1] = y; a_pMat->m[3][2] = z; a_pMat->m[3][3] = 1;
}

inline
void InitTransMat4f( Mat4f *a_pMat, Vec3f *a_pTrans )
{
	a_pMat->m[0][0] = 1;           a_pMat->m[0][1] = 0;           a_pMat->m[0][2] = 0;           a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;           a_pMat->m[1][1] = 1;           a_pMat->m[1][2] = 0;           a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;           a_pMat->m[2][1] = 0;           a_pMat->m[2][2] = 1;           a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = a_pTrans->x; a_pMat->m[3][1] = a_pTrans->y; a_pMat->m[3][2] = a_pTrans->z; a_pMat->m[3][3] = 1;
}

/*
inline
void InitRotXMat4f( Mat4f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0;                        a_pMat->m[0][2] = 0;                       a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = cosf(angle*PI_F/180.0f);  a_pMat->m[1][2] = sinf(angle*PI_F/180.0f); a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = -sinf(angle*PI_F/180.0f); a_pMat->m[2][2] = cosf(angle*PI_F/180.0f); a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0; a_pMat->m[3][1] = 0;                        a_pMat->m[3][2] = 0;                       a_pMat->m[3][3] = 1;
}

inline
void InitRotYMat4f( Mat4f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = cosf(angle*PI_F/180.0f);  a_pMat->m[0][1] = 0; a_pMat->m[0][2] = -sinf(angle*PI_F/180.0f); a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;                        a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0;                        a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = sinf(angle*PI_F/180.0f);  a_pMat->m[2][1] = 0; a_pMat->m[2][2] = cosf(angle*PI_F/180.0f);  a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0;                        a_pMat->m[3][1] = 0; a_pMat->m[3][2] = 0;                        a_pMat->m[3][3] = 1;
}

inline
void InitRotZMat4f( Mat4f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = cosf(angle*PI_F/180.0f);  a_pMat->m[0][1] = sinf(angle*PI_F/180.0f); a_pMat->m[0][2] = 0; a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = -sinf(angle*PI_F/180.0f); a_pMat->m[1][1] = cosf(angle*PI_F/180.0f); a_pMat->m[1][2] = 0; a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;                        a_pMat->m[2][1] = 0; 					   a_pMat->m[2][2] = 1; a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0;                        a_pMat->m[3][1] = 0;                       a_pMat->m[3][2] = 0; a_pMat->m[3][3] = 1;
}
*/

inline
void InitRotArbAxisMat4f( Mat4f *a_pMat, Vec3f *a_pAxis, f32 angle )
{
	f32 c = cosf(angle*PI_F/180.0f);
	f32 mC = 1.0f-c;
	f32 s = sinf(angle*PI_F/180.0f);
	a_pMat->m[0][0] = c                          + (a_pAxis->x*a_pAxis->x*mC); a_pMat->m[0][1] = (a_pAxis->y*a_pAxis->x*mC) + (a_pAxis->z*s);             a_pMat->m[0][2] = (a_pAxis->z*a_pAxis->x*mC) - (a_pAxis->y*s);             a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = (a_pAxis->x*a_pAxis->y*mC) - (a_pAxis->z*s);             a_pMat->m[1][1] = c                          + (a_pAxis->y*a_pAxis->y*mC); a_pMat->m[1][2] = (a_pAxis->z*a_pAxis->y*mC) + (a_pAxis->x*s);             a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = (a_pAxis->x*a_pAxis->z*mC) + (a_pAxis->y*s);             a_pMat->m[2][1] = (a_pAxis->y*a_pAxis->z*mC) - (a_pAxis->x*s);             a_pMat->m[2][2] = c                          + (a_pAxis->z*a_pAxis->z*mC); a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0;                                                       a_pMat->m[3][1] = 0;                                                       a_pMat->m[3][2] = 0;                                                       a_pMat->m[3][3] = 1;
}

/*
//can't use near and far cause of stupid msvc https://stackoverflow.com/questions/3869830/near-and-far-pointers/3869852
//Following is OpenGL, but i need to verify that it is what the OpenGL standard gives!
inline
void InitPerspectiveProjectionMat4fOpenGL( Mat4f *a_pMat, u64 width, u64 height, f32 a_hFOV, f32 a_vFOV, f32 nearPlane, f32 farPlane )
{
	f32 thFOV = tanf(a_hFOV*0.5f);
	f32 tvFOV = tanf(a_vFOV*0.5f);
	f64 dNearPlane = (f64)nearPlane;
	f64 dFarPlane = (f64)farPlane;
	f64 nMinF = (dNearPlane-dFarPlane);
  	f32 aspect = height / (f32)width;
	a_pMat->m[0][0] = aspect/(thFOV*thFOV); a_pMat->m[0][1] = 0;                  a_pMat->m[0][2] = 0;                               		 a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;                    a_pMat->m[1][1] = 1.0f/(tvFOV*tvFOV); a_pMat->m[1][2] = 0;                               		 a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;                    a_pMat->m[2][1] = 0;                  a_pMat->m[2][2] = (f32)((dFarPlane+dNearPlane)/nMinF);     a_pMat->m[2][3] = -1.0f;
	a_pMat->m[3][0] = 0;                    a_pMat->m[3][1] = 0;                  a_pMat->m[3][2] = (f32)(2.0*(dFarPlane*dNearPlane)/nMinF); a_pMat->m[3][3] = 0;
}
inline
void InitInvPerspectiveProjectionMat4fOpenGL( Mat4f *a_pMat, u64 width, u64 height, f32 a_hFOV, f32 a_vFOV, f32 nearPlane, f32 farPlane )
{
	f32 thFOV = tanf(a_hFOV*0.5f);
	f32 tvFOV = tanf(a_vFOV*0.5f);
	f64 dNearPlane = (f64)nearPlane;
	f64 dFarPlane = (f64)farPlane;
	f64 nMinF = (dNearPlane-dFarPlane);
	f64 fFarNearDoubled = (2.0*(dFarPlane*dNearPlane));
  	f32 invAspect = (f32)width/height;
	a_pMat->m[0][0] = thFOV*thFOV*invAspect; 		   a_pMat->m[0][1] = 0;             a_pMat->m[0][2] = 0;     a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;                               a_pMat->m[1][1] = tvFOV * tvFOV; a_pMat->m[1][2] = 0;     a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;                               a_pMat->m[2][1] = 0;             a_pMat->m[2][2] = 0;     a_pMat->m[2][3] = (f32)(nMinF/fFarNearDoubled);
	a_pMat->m[3][0] = 0;                               a_pMat->m[3][1] = 0;             a_pMat->m[3][2] = -1.0f; a_pMat->m[3][3] = (f32)((dFarPlane+dNearPlane)/fFarNearDoubled);
}
*/

//Following are DirectX Matrices
inline
void InitPerspectiveProjectionMat4fDirectXRH( Mat4f *a_pMat, u64 width, u64 height, f32 a_hFOV, f32 a_vFOV, f32 nearPlane, f32 farPlane )
{
	f32 thFOV = tanf(a_hFOV*PI_F/360);
	f32 tvFOV = tanf(a_vFOV*PI_F/360);
	f32 nMinF = farPlane/(nearPlane-farPlane);
  	f32 aspect = height / (f32)width;
	a_pMat->m[0][0] = aspect/(thFOV); a_pMat->m[0][1] = 0;            a_pMat->m[0][2] = 0;               a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;              a_pMat->m[1][1] = 1.0f/(tvFOV); a_pMat->m[1][2] = 0;               a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;              a_pMat->m[2][1] = 0;            a_pMat->m[2][2] = nMinF;           a_pMat->m[2][3] = -1.0f;
	a_pMat->m[3][0] = 0;              a_pMat->m[3][1] = 0;            a_pMat->m[3][2] = nearPlane*nMinF; a_pMat->m[3][3] = 0;
}

inline
void InitPerspectiveProjectionMat4fDirectXLH( Mat4f *a_pMat, u64 width, u64 height, f32 a_hFOV, f32 a_vFOV, f32 nearPlane, f32 farPlane )
{
	f32 thFOV = tanf(a_hFOV*PI_F/360);
	f32 tvFOV = tanf(a_vFOV*PI_F/360);
	f32 nMinF = farPlane/(nearPlane-farPlane);
  	f32 aspect = height / (f32)width;
	a_pMat->m[0][0] = aspect/(thFOV); a_pMat->m[0][1] = 0;            a_pMat->m[0][2] = 0;               a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;              a_pMat->m[1][1] = 1.0f/(tvFOV); a_pMat->m[1][2] = 0;               a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;              a_pMat->m[2][1] = 0;            a_pMat->m[2][2] = -nMinF;          a_pMat->m[2][3] = 1.0f;
	a_pMat->m[3][0] = 0;              a_pMat->m[3][1] = 0;            a_pMat->m[3][2] = nearPlane*nMinF; a_pMat->m[3][3] = 0;
}

/*
inline
void InitPerspectiveProjectionMat4fOculusDirectXLH( Mat4f *a_pMat, ovrFovPort tanHalfFov, f32 nearPlane, f32 farPlane )
{
    f32 projXScale = 2.0f / ( tanHalfFov.LeftTan + tanHalfFov.RightTan );
    f32 projXOffset = ( tanHalfFov.LeftTan - tanHalfFov.RightTan ) * projXScale * 0.5f;
    f32 projYScale = 2.0f / ( tanHalfFov.UpTan + tanHalfFov.DownTan );
    f32 projYOffset = ( tanHalfFov.UpTan - tanHalfFov.DownTan ) * projYScale * 0.5f;
	f32 nMinF = farPlane/(nearPlane-farPlane);
	a_pMat->m[0][0] = projXScale;  a_pMat->m[0][1] = 0;            a_pMat->m[0][2] = 0;               a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;           a_pMat->m[1][1] = projYScale;   a_pMat->m[1][2] = 0;               a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = projXOffset; a_pMat->m[2][1] = -projYOffset; a_pMat->m[2][2] = -nMinF;          a_pMat->m[2][3] = 1.0f;
	a_pMat->m[3][0] = 0;           a_pMat->m[3][1] = 0;            a_pMat->m[3][2] = nearPlane*nMinF; a_pMat->m[3][3] = 0;
}


inline
void InitPerspectiveProjectionMat4fOculusDirectXRH( Mat4f *a_pMat, ovrFovPort tanHalfFov, f32 nearPlane, f32 farPlane )
{
    f32 projXScale = 2.0f / ( tanHalfFov.LeftTan + tanHalfFov.RightTan );
    f32 projXOffset = ( tanHalfFov.LeftTan - tanHalfFov.RightTan ) * projXScale * 0.5f;
    f32 projYScale = 2.0f / ( tanHalfFov.UpTan + tanHalfFov.DownTan );
    f32 projYOffset = ( tanHalfFov.UpTan - tanHalfFov.DownTan ) * projYScale * 0.5f;
	f32 nMinF = farPlane/(nearPlane-farPlane);
	a_pMat->m[0][0] = projXScale;   a_pMat->m[0][1] = 0;           a_pMat->m[0][2] = 0;               a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;            a_pMat->m[1][1] = projYScale;  a_pMat->m[1][2] = 0;               a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = -projXOffset; a_pMat->m[2][1] = projYOffset; a_pMat->m[2][2] = nMinF;           a_pMat->m[2][3] = -1.0f;
	a_pMat->m[3][0] = 0;            a_pMat->m[3][1] = 0;           a_pMat->m[3][2] = nearPlane*nMinF; a_pMat->m[3][3] = 0;
}
*/

inline
f32 DeterminantUpper3x3Mat4f( Mat4f *a_pMat )
{
	return (a_pMat->m[0][0] * ((a_pMat->m[1][1]*a_pMat->m[2][2]) - (a_pMat->m[1][2]*a_pMat->m[2][1]))) + 
		   (a_pMat->m[0][1] * ((a_pMat->m[2][0]*a_pMat->m[1][2]) - (a_pMat->m[1][0]*a_pMat->m[2][2]))) + 
		   (a_pMat->m[0][2] * ((a_pMat->m[1][0]*a_pMat->m[2][1]) - (a_pMat->m[2][0]*a_pMat->m[1][1])));
}

inline
void InverseUpper3x3Mat4f( Mat4f *__restrict a_pMat, Mat4f *__restrict out )
{
	f32 fDet = DeterminantUpper3x3Mat4f( a_pMat );
#if MAIN_DEBUG
	assert( fDet != 0.f );
#endif
	f32 fInvDet = 1.0f / fDet;
	out->m[0][0] = fInvDet * ((a_pMat->m[1][1]*a_pMat->m[2][2]) - (a_pMat->m[1][2]*a_pMat->m[2][1]));
	out->m[0][1] = fInvDet * ((a_pMat->m[0][2]*a_pMat->m[2][1]) - (a_pMat->m[0][1]*a_pMat->m[2][2]));
	out->m[0][2] = fInvDet * ((a_pMat->m[0][1]*a_pMat->m[1][2]) - (a_pMat->m[0][2]*a_pMat->m[1][1]));
	out->m[0][3] = 0.0f;

	out->m[1][0] = fInvDet * ((a_pMat->m[2][0]*a_pMat->m[1][2]) - (a_pMat->m[2][2]*a_pMat->m[1][0]));
	out->m[1][1] = fInvDet * ((a_pMat->m[0][0]*a_pMat->m[2][2]) - (a_pMat->m[0][2]*a_pMat->m[2][0])); 
	out->m[1][2] = fInvDet * ((a_pMat->m[0][2]*a_pMat->m[1][0]) - (a_pMat->m[1][2]*a_pMat->m[0][0]));
	out->m[1][3] = 0.0f;

	out->m[2][0] = fInvDet * ((a_pMat->m[1][0]*a_pMat->m[2][1]) - (a_pMat->m[1][1]*a_pMat->m[2][0]));
	out->m[2][1] = fInvDet * ((a_pMat->m[0][1]*a_pMat->m[2][0]) - (a_pMat->m[0][0]*a_pMat->m[2][1]));
	out->m[2][2] = fInvDet * ((a_pMat->m[0][0]*a_pMat->m[1][1]) - (a_pMat->m[1][0]*a_pMat->m[0][1]));
	out->m[2][3] = 0.0f;

	out->m[3][0] = 0.0f;
	out->m[3][1] = 0.0f;
	out->m[3][2] = 0.0f;
	out->m[3][3] = 1.0f;
}

inline
void InverseTransposeUpper3x3Mat4f( Mat4f *__restrict a_pMat, Mat4f *__restrict out )
{
	f32 fDet = DeterminantUpper3x3Mat4f( a_pMat );
#if MAIN_DEBUG
	assert( fDet != 0.f );
#endif
	f32 fInvDet = 1.0f / fDet;
	out->m[0][0] = fInvDet * ((a_pMat->m[1][1]*a_pMat->m[2][2]) - (a_pMat->m[1][2]*a_pMat->m[2][1]));
	out->m[0][1] = fInvDet * ((a_pMat->m[2][0]*a_pMat->m[1][2]) - (a_pMat->m[2][2]*a_pMat->m[1][0]));
	out->m[0][2] = fInvDet * ((a_pMat->m[1][0]*a_pMat->m[2][1]) - (a_pMat->m[1][1]*a_pMat->m[2][0]));
	out->m[0][3] = 0.0f;

	out->m[1][0] = fInvDet * ((a_pMat->m[0][2]*a_pMat->m[2][1]) - (a_pMat->m[0][1]*a_pMat->m[2][2]));
	out->m[1][1] = fInvDet * ((a_pMat->m[0][0]*a_pMat->m[2][2]) - (a_pMat->m[0][2]*a_pMat->m[2][0])); 
	out->m[1][2] = fInvDet * ((a_pMat->m[0][1]*a_pMat->m[2][0]) - (a_pMat->m[0][0]*a_pMat->m[2][1]));
	out->m[1][3] = 0.0f;

	out->m[2][0] = fInvDet * ((a_pMat->m[0][1]*a_pMat->m[1][2]) - (a_pMat->m[0][2]*a_pMat->m[1][1]));
	out->m[2][1] = fInvDet * ((a_pMat->m[0][2]*a_pMat->m[1][0]) - (a_pMat->m[1][2]*a_pMat->m[0][0]));
	out->m[2][2] = fInvDet * ((a_pMat->m[0][0]*a_pMat->m[1][1]) - (a_pMat->m[1][0]*a_pMat->m[0][1]));
	out->m[2][3] = 0.0f;

	out->m[3][0] = 0.0f;
	out->m[3][1] = 0.0f;
	out->m[3][2] = 0.0f;
	out->m[3][3] = 1.0f;
}

inline
void InverseTransposeUpper3x3Mat4f( Mat4f *__restrict a_pMat, Mat3x4f *__restrict out )
{
	f32 fDet = DeterminantUpper3x3Mat4f( a_pMat );
#if MAIN_DEBUG
	assert( fDet != 0.f );
#endif
	f32 fInvDet = 1.0f / fDet;
	out->m[0][0] = fInvDet * ((a_pMat->m[1][1]*a_pMat->m[2][2]) - (a_pMat->m[1][2]*a_pMat->m[2][1]));
	out->m[0][1] = fInvDet * ((a_pMat->m[2][0]*a_pMat->m[1][2]) - (a_pMat->m[2][2]*a_pMat->m[1][0]));
	out->m[0][2] = fInvDet * ((a_pMat->m[1][0]*a_pMat->m[2][1]) - (a_pMat->m[1][1]*a_pMat->m[2][0]));
	out->m[0][3] = 0.0f;

	out->m[1][0] = fInvDet * ((a_pMat->m[0][2]*a_pMat->m[2][1]) - (a_pMat->m[0][1]*a_pMat->m[2][2]));
	out->m[1][1] = fInvDet * ((a_pMat->m[0][0]*a_pMat->m[2][2]) - (a_pMat->m[0][2]*a_pMat->m[2][0])); 
	out->m[1][2] = fInvDet * ((a_pMat->m[0][1]*a_pMat->m[2][0]) - (a_pMat->m[0][0]*a_pMat->m[2][1]));
	out->m[1][3] = 0.0f;

	out->m[2][0] = fInvDet * ((a_pMat->m[0][1]*a_pMat->m[1][2]) - (a_pMat->m[0][2]*a_pMat->m[1][1]));
	out->m[2][1] = fInvDet * ((a_pMat->m[0][2]*a_pMat->m[1][0]) - (a_pMat->m[1][2]*a_pMat->m[0][0]));
	out->m[2][2] = fInvDet * ((a_pMat->m[0][0]*a_pMat->m[1][1]) - (a_pMat->m[1][0]*a_pMat->m[0][1]));
	out->m[2][3] = 0.0f;
}


inline
void Mat4fMult( Mat4f *__restrict a, Mat4f *__restrict b, Mat4f *__restrict out)
{
	out->m[0][0] = a->m[0][0]*b->m[0][0] + a->m[0][1]*b->m[1][0] + a->m[0][2]*b->m[2][0] + a->m[0][3]*b->m[3][0];
	out->m[0][1] = a->m[0][0]*b->m[0][1] + a->m[0][1]*b->m[1][1] + a->m[0][2]*b->m[2][1] + a->m[0][3]*b->m[3][1];
	out->m[0][2] = a->m[0][0]*b->m[0][2] + a->m[0][1]*b->m[1][2] + a->m[0][2]*b->m[2][2] + a->m[0][3]*b->m[3][2];
	out->m[0][3] = a->m[0][0]*b->m[0][3] + a->m[0][1]*b->m[1][3] + a->m[0][2]*b->m[2][3] + a->m[0][3]*b->m[3][3];

	out->m[1][0] = a->m[1][0]*b->m[0][0] + a->m[1][1]*b->m[1][0] + a->m[1][2]*b->m[2][0] + a->m[1][3]*b->m[3][0];
	out->m[1][1] = a->m[1][0]*b->m[0][1] + a->m[1][1]*b->m[1][1] + a->m[1][2]*b->m[2][1] + a->m[1][3]*b->m[3][1];
	out->m[1][2] = a->m[1][0]*b->m[0][2] + a->m[1][1]*b->m[1][2] + a->m[1][2]*b->m[2][2] + a->m[1][3]*b->m[3][2];
	out->m[1][3] = a->m[1][0]*b->m[0][3] + a->m[1][1]*b->m[1][3] + a->m[1][2]*b->m[2][3] + a->m[1][3]*b->m[3][3];

	out->m[2][0] = a->m[2][0]*b->m[0][0] + a->m[2][1]*b->m[1][0] + a->m[2][2]*b->m[2][0] + a->m[2][3]*b->m[3][0];
	out->m[2][1] = a->m[2][0]*b->m[0][1] + a->m[2][1]*b->m[1][1] + a->m[2][2]*b->m[2][1] + a->m[2][3]*b->m[3][1];
	out->m[2][2] = a->m[2][0]*b->m[0][2] + a->m[2][1]*b->m[1][2] + a->m[2][2]*b->m[2][2] + a->m[2][3]*b->m[3][2];
	out->m[2][3] = a->m[2][0]*b->m[0][3] + a->m[2][1]*b->m[1][3] + a->m[2][2]*b->m[2][3] + a->m[2][3]*b->m[3][3];

	out->m[3][0] = a->m[3][0]*b->m[0][0] + a->m[3][1]*b->m[1][0] + a->m[3][2]*b->m[2][0] + a->m[3][3]*b->m[3][0];
	out->m[3][1] = a->m[3][0]*b->m[0][1] + a->m[3][1]*b->m[1][1] + a->m[3][2]*b->m[2][1] + a->m[3][3]*b->m[3][1];
	out->m[3][2] = a->m[3][0]*b->m[0][2] + a->m[3][1]*b->m[1][2] + a->m[3][2]*b->m[2][2] + a->m[3][3]*b->m[3][2];
	out->m[3][3] = a->m[3][0]*b->m[0][3] + a->m[3][1]*b->m[1][3] + a->m[3][2]*b->m[2][3] + a->m[3][3]*b->m[3][3];
}

inline
void Vec3fAdd( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = a->x + b->x;
	out->y = a->y + b->y;
	out->z = a->z + b->z;
}

inline
void Vec3fSub( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = a->x - b->x;
	out->y = a->y - b->y;
	out->z = a->z - b->z;
}

inline
void Vec3fMult( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = a->x * b->x;
	out->y = a->y * b->y;
	out->z = a->z * b->z;
}

inline
void Vec3fCross( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = (a->y * b->z) - (a->z * b->y);
	out->y = (a->z * b->x) - (a->x * b->z);
	out->z = (a->x * b->y) - (a->y * b->x);
}

inline
void Vec3fScale( Vec3f *a, f32 scale, Vec3f *out )
{
	out->x = a->x * scale;
	out->y = a->y * scale;
	out->z = a->z * scale;
}


inline
void Vec3fScaleAdd( Vec3f *a, f32 scale, Vec3f *b, Vec3f *out )
{
	out->x = (a->x * scale) + b->x;
	out->y = (a->y * scale) + b->y;
	out->z = (a->z * scale) + b->z;
}

inline
f32 Vec3fDot( Vec3f *a, Vec3f *b )
{
	return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

inline
void Vec3fNormalize( Vec3f *a, Vec3f *out )
{

	f32 mag = sqrtf((a->x*a->x) + (a->y*a->y) + (a->z*a->z));
	if(mag == 0)
	{
		out->x = 0;
		out->y = 0;
		out->z = 0;
	}
	else
	{
		out->x = a->x/mag;
		out->y = a->y/mag;
		out->z = a->z/mag;
	}
}


inline
void Vec3fLerp( Vec3f *a, Vec3f *b, f32 fT, Vec3f *out )
{
	Vec3f vTmp;
	Vec3fSub(b,a,&vTmp);
	Vec3fScaleAdd(&vTmp,fT,a,out);
}

inline
void Vec3fRotByUnitQuat(Vec3f *v, Quatf *__restrict q, Vec3f *out)
{
    f32 fVecScalar = (2.0f*q->w*q->w)-1;
    f32 fQuatVecScalar = 2.0f* Vec3fDot(v,&q->v);

    Vec3f vScaledQuatVec;
    Vec3f vScaledVec;
    Vec3fScale(&q->v,fQuatVecScalar,&vScaledQuatVec);
    Vec3fScale(v,fVecScalar,&vScaledVec);

    Vec3f vQuatCrossVec;
    Vec3fCross(&q->v, v, &vQuatCrossVec);

    Vec3fScale(&vQuatCrossVec,2.0f*q->w,&vQuatCrossVec);

    Vec3fAdd(&vScaledQuatVec,&vScaledVec,out);
    Vec3fAdd(out,&vQuatCrossVec,out);
}

/*
inline
void Vec3fRotByUnitQuat(Vec3f *v, Quatf *__restrict q, Vec3f *out)
{
	Vec3f vDoubleRot;
	vDoubleRot.x = q->x + q->x;
	vDoubleRot.y = q->y + q->y;
	vDoubleRot.z = q->z + q->z;

	Vec3f vScaledWRot;
	vScaledWRot.x = q->w * vDoubleRot.x;
	vScaledWRot.y = q->w * vDoubleRot.y;
	vScaledWRot.z = q->w * vDoubleRot.z;

	Vec3f vScaledXRot;
	vScaledXRot.x = q->x * vDoubleRot.x;
	vScaledXRot.y = q->x * vDoubleRot.y;
	vScaledXRot.z = q->x * vDoubleRot.z;

	f32 fScaledYRot0 = q->y * vDoubleRot.y;
	f32 fScaledYRot1 = q->y * vDoubleRot.z;

	f32 fScaledZRot0 = q->z * vDoubleRot.z;

	out->x = ((v->x * ((1.f - fScaledYRot0) - fScaledZRot0)) + (v->y * (vScaledXRot.y - vScaledWRot.z))) + (v->z * (vScaledXRot.z + vScaledWRot.y));
	out->y = ((v->x * (vScaledXRot.y + vScaledWRot.z)) + (v->y * ((1.f - vScaledXRot.x) - fScaledZRot0))) + (v->z * (fScaledYRot1 - vScaledWRot.x));
	out->z = ((v->x * (vScaledXRot.z - vScaledWRot.y)) + (v->y * (fScaledYRot1 + vScaledWRot.x))) + (v->z * ((1.f - vScaledXRot.x) - fScaledYRot0));
}
*/


inline
void InitUnitQuatf( Quatf *q, f32 angle, Vec3f *axis )
{
	f32 s = sinf(angle*PI_F/360.0f);
	q->w = cosf(angle*PI_F/360.0f);
	q->x = axis->x * s;
	q->y = axis->y * s;
	q->z = axis->z * s;
}

inline
void QuatfMult( Quatf *__restrict a, Quatf *__restrict b, Quatf *__restrict out )
{
	out->w = (a->w * b->w) - (a->x* b->x) - (a->y* b->y) - (a->z* b->z);
	out->x = (a->w * b->x) + (a->x* b->w) + (a->y* b->z) - (a->z* b->y);
	out->y = (a->w * b->y) + (a->y* b->w) + (a->z* b->x) - (a->x* b->z);
	out->z = (a->w * b->z) + (a->z* b->w) + (a->x* b->y) - (a->y* b->x);
}

inline
void QuatfSub( Quatf *a, Quatf *b, Quatf *out )
{
	out->w = a->w - b->w;
	out->x = a->x - b->x;
	out->y = a->y - b->y;
	out->z = a->z - b->z;
}

inline
void QuatfScaleAdd( Quatf *a, f32 scale, Quatf *b, Quatf *out )
{
	out->w = (a->w * scale) + b->w;
	out->x = (a->x * scale) + b->x;
	out->y = (a->y * scale) + b->y;
	out->z = (a->z * scale) + b->z;
}

inline
void QuatfNormalize( Quatf *a, Quatf *out )
{

	f32 mag = sqrtf((a->w*a->w) + (a->x*a->x) + (a->y*a->y) + (a->z*a->z));
	if(mag == 0.f)
	{
		out->w = 0.f;
		out->x = 0.f;
		out->y = 0.f;
		out->z = 0.f;
	}
	else
	{
		out->w = a->w/mag;
		out->x = a->x/mag;
		out->y = a->y/mag;
		out->z = a->z/mag;
	}
}

//todo simplify to reduce floating point error
inline
void InitViewMat4ByQuatf( Mat4f *a_pMat, Quatf *a_qRot, Vec3f *a_pPos )
{
	a_pMat->m[0][0] = 1.0f - 2.0f*(a_qRot->y*a_qRot->y + a_qRot->z*a_qRot->z);                            a_pMat->m[0][1] = 2.0f*(a_qRot->x*a_qRot->y - a_qRot->w*a_qRot->z);                                   a_pMat->m[0][2] = 2.0f*(a_qRot->x*a_qRot->z + a_qRot->w*a_qRot->y);        		                      a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 2.0f*(a_qRot->x*a_qRot->y + a_qRot->w*a_qRot->z);                                   a_pMat->m[1][1] = 1.0f - 2.0f*(a_qRot->x*a_qRot->x + a_qRot->z*a_qRot->z);                            a_pMat->m[1][2] = 2.0f*(a_qRot->y*a_qRot->z - a_qRot->w*a_qRot->x);        		                      a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 2.0f*(a_qRot->x*a_qRot->z - a_qRot->w*a_qRot->y);                                   a_pMat->m[2][1] = 2.0f*(a_qRot->y*a_qRot->z + a_qRot->w*a_qRot->x);                                   a_pMat->m[2][2] = 1.0f - 2.0f*(a_qRot->x*a_qRot->x + a_qRot->y*a_qRot->y); 		                      a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = -a_pPos->x*a_pMat->m[0][0] - a_pPos->y*a_pMat->m[1][0] - a_pPos->z*a_pMat->m[2][0]; a_pMat->m[3][1] = -a_pPos->x*a_pMat->m[0][1] - a_pPos->y*a_pMat->m[1][1] - a_pPos->z*a_pMat->m[2][1]; a_pMat->m[3][2] = -a_pPos->x*a_pMat->m[0][2] - a_pPos->y*a_pMat->m[1][2] - a_pPos->z*a_pMat->m[2][2]; a_pMat->m[3][3] = 1;
}

inline
void InitModelMat4ByQuatf( Mat4f *a_pMat, Quatf *a_qRot, Vec3f *a_pPos )
{
	a_pMat->m[0][0] = 1.0f - 2.0f*(a_qRot->y*a_qRot->y + a_qRot->z*a_qRot->z);                            a_pMat->m[0][1] = 2.0f*(a_qRot->x*a_qRot->y + a_qRot->w*a_qRot->z);                                   a_pMat->m[0][2] = 2.0f*(a_qRot->x*a_qRot->z - a_qRot->w*a_qRot->y);        		                      a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 2.0f*(a_qRot->x*a_qRot->y - a_qRot->w*a_qRot->z);                                   a_pMat->m[1][1] = 1.0f - 2.0f*(a_qRot->x*a_qRot->x + a_qRot->z*a_qRot->z);                            a_pMat->m[1][2] = 2.0f*(a_qRot->y*a_qRot->z + a_qRot->w*a_qRot->x);        		                      a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 2.0f*(a_qRot->x*a_qRot->z + a_qRot->w*a_qRot->y);                                   a_pMat->m[2][1] = 2.0f*(a_qRot->y*a_qRot->z - a_qRot->w*a_qRot->x);                                   a_pMat->m[2][2] = 1.0f - 2.0f*(a_qRot->x*a_qRot->x + a_qRot->y*a_qRot->y); 		                      a_pMat->m[2][3] = 0;
	//a_pMat->m[3][0] = a_pPos->x*a_pMat->m[0][0] + a_pPos->y*a_pMat->m[1][0] + a_pPos->z*a_pMat->m[2][0];  a_pMat->m[3][1] = a_pPos->x*a_pMat->m[0][1] + a_pPos->y*a_pMat->m[1][1] + a_pPos->z*a_pMat->m[2][1]; a_pMat->m[3][2] = a_pPos->x*a_pMat->m[0][2] + a_pPos->y*a_pMat->m[1][2] + a_pPos->z*a_pMat->m[2][2]; a_pMat->m[3][3] = 1;
	a_pMat->m[3][0] = a_pPos->x;  a_pMat->m[3][1] = a_pPos->y; a_pMat->m[3][2] = a_pPos->z; a_pMat->m[3][3] = 1.f;
}

inline
void QuatfNormLerp( Quatf *a, Quatf *b, f32 fT, Quatf *out )
{
	Quatf qTmp;
	QuatfSub(b,a,&qTmp);
	QuatfScaleAdd(&qTmp,fT,a,out);
	QuatfNormalize(out,out);
}

inline
void QuatfSlerp( Vec3f *a, Vec3f *b, f32 fT, Vec3f *out )
{
	//todo
}

#if MAIN_DEBUG
void PrintMat4f( Mat4f *a_pMat )
{
	for( u32 dwIdx = 0; dwIdx < 4; ++dwIdx )
	{
		for( u32 dwJdx = 0; dwJdx < 4; ++dwJdx )
		{
			printf("%f ", a_pMat->m[dwIdx][dwJdx] );
		}
		printf("\n");
	}
}
#endif

f32 clamp(f32 d, f32 min, f32 max) {
  const f32 t = d < min ? min : d;
  return t > max ? max : t;
}

u32 max( u32 a, u32 b )
{
	return a > b ? a : b;
}

//TODO CHANGE TO A POPUP AND FILE LOG
int logWindowsError(const char* msg)
{
#if MAIN_DEBUG
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR)&lpMsgBuf, 0, NULL );
    OutputDebugStringA( msg );
    OutputDebugStringA( (LPCTSTR)lpMsgBuf );

    LocalFree( lpMsgBuf );
    //MessageBoxW(NULL, L"Error creating DXGI factory", L"Error", MB_OK | MB_ICONERROR);
#endif
    return -1;
}

int logError(const char* msg)
{
#if MAIN_DEBUG
	MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
#endif
    return -1;
}

#if MAIN_DEBUG
inline
bool EnableDebugLayer()
{
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    if ( FAILED( D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) ) ) )
    {
    	logError( "Error creating DirectX12 Debug layer\n" );  
        return false;
    }
    debugInterface->EnableDebugLayer();
    return true;
}

inline
void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}
#endif

inline
ID3D12CommandQueue *InitCopyCommandQueue( ID3D12Device2* dxd3Device, u32 dwGPUNumber )
{
	ID3D12CommandQueue *cq;
	D3D12_COMMAND_QUEUE_DESC cqDesc;
    cqDesc.Type =     D3D12_COMMAND_LIST_TYPE_COPY;
    cqDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cqDesc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqDesc.NodeMask = dwGPUNumber;

    if( FAILED( dxd3Device->CreateCommandQueue( &cqDesc, IID_PPV_ARGS( &cq ) ) ) )
	{
		return NULL;
	}
	return cq;
}

inline
ID3D12CommandQueue *InitComputeCommandQueue( ID3D12Device2* dxd3Device, u32 dwGPUNumber )
{
	ID3D12CommandQueue *cq;
	D3D12_COMMAND_QUEUE_DESC cqDesc;
    cqDesc.Type =     D3D12_COMMAND_LIST_TYPE_COMPUTE;
    cqDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cqDesc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqDesc.NodeMask = dwGPUNumber;

    if( FAILED( dxd3Device->CreateCommandQueue( &cqDesc, IID_PPV_ARGS( &cq ) ) ) )
	{
		return NULL;
	}
	return cq;
}

inline
void UploadModels( u32 dwGPUNumber, u32 dwVisibleGPUMask )
{
	f32 planeVertices[] =
	{
	        // positions              // vertex norms    // vertex colors
	         1000.0f,  -1.0f,  1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	        -1000.0f,  -1.0f,  1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	         1000.0f,  -1.0f, -1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	        -1000.0f,  -1.0f, -1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	         1000.0f,  -1.0f,  1000.0f, 0.0f, -1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	        -1000.0f,  -1.0f,  1000.0f, 0.0f, -1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	         1000.0f,  -1.0f, -1000.0f, 0.0f, -1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
	        -1000.0f,  -1.0f, -1000.0f, 0.0f, -1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f
	};
	
	u32 planeIndices[] = 
	{
	        0, 1, 2,
	        2, 1, 3,
	        4, 6, 5,
	        6, 7, 5
	};
	planeIndexCount = 12;
	
	f32 cubeVertices[] = {
	    // positions          // vertex norms     //vertex colors
	    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	
	    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	
	    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	
	     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	
	    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	
	    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f
	};
	
	u32 cubeIndicies[] =
	{
	     0,  1,  2, //back
	     3,  4,  5,
	
	     6,  7,  8,
	     9, 10, 11,
	
	    12, 13, 14,
	    15, 16, 17,
	
	    18, 20, 19,
	    21, 23, 22,
	
	    24, 25, 26, //bottom
	    27, 28, 29,
	
	    30, 31, 32, //top
	    33, 34, 35
	};
	cubeIndexCount = 36;


	const u64 qwModelSize = sizeof(planeVertices) + sizeof(planeIndices) + sizeof(cubeVertices) + sizeof(cubeIndicies);

	D3D12_RESOURCE_DESC resourceBufferDesc; //describes what is placed in heap
  	resourceBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  	resourceBufferDesc.Alignment = 0;
  	resourceBufferDesc.Width = qwModelSize;
  	resourceBufferDesc.Height = 1;
  	resourceBufferDesc.DepthOrArraySize = 1;
  	resourceBufferDesc.MipLevels = 1;
  	resourceBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
  	resourceBufferDesc.SampleDesc.Count = 1;
  	resourceBufferDesc.SampleDesc.Quality = 0;
  	resourceBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  	resourceBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE; //D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE

	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo( dwVisibleGPUMask, 1, &resourceBufferDesc );
	u64 qwNumFullAlignments = qwModelSize / allocInfo.Alignment;
	const u64 qwAlignedModelSize = (qwNumFullAlignments * allocInfo.Alignment) + ((qwModelSize % allocInfo.Alignment) > 0 ? allocInfo.Alignment : 0);
	const u64 qwHeapSize = qwAlignedModelSize;

	//https://zhangdoa.com/posts/walking-through-the-heap-properties-in-directx-12
	//https://asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment
	//https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier#D3D12_RESOURCE_HEAP_TIER_1
	// we only allow buffers to stay within the first tier heap tier
	D3D12_HEAP_DESC modelHeapDesc;
	modelHeapDesc.SizeInBytes = qwHeapSize;
	modelHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	modelHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	modelHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	modelHeapDesc.Properties.CreationNodeMask = dwGPUNumber;
	modelHeapDesc.Properties.VisibleNodeMask = dwVisibleGPUMask;
	modelHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; //64KB heap alignment, SizeInBytes should be a multiple of the heap alignment. is 64KB here 65536
	modelHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

	device->CreateHeap( &modelHeapDesc, IID_PPV_ARGS(&pModelDefaultHeap) );
#if MAIN_DEBUG
	pModelDefaultHeap->SetName( L"Model Buffer Default Resource Heap" );
#endif

	modelHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;

	device->CreateHeap( &modelHeapDesc, IID_PPV_ARGS(&pModelUploadHeap) );
#if MAIN_DEBUG
	pModelUploadHeap->SetName( L"Model Buffer Upload Resource Heap" );
#endif

  	//verify that we are using the advanced model!
	device->CreatePlacedResource( pModelUploadHeap, 0, &resourceBufferDesc,D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer) );
	device->CreatePlacedResource( pModelDefaultHeap, 0, &resourceBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer) );

    //upload to upload heap (TODO is there a penalty from crossing a buffer alignment boundary with mesh data?)
    //TODO is there a penatly for not having meshes at an alignment or their own resouce?
    u8* pUploadBufferData;
    if( FAILED( uploadBuffer->Map( 0, nullptr, (void**) &pUploadBufferData ) ) )
    {
        return;
    }
    memcpy(pUploadBufferData,planeVertices,sizeof(planeVertices));
    memcpy(pUploadBufferData+sizeof(planeVertices),planeIndices,sizeof(planeIndices));
    memcpy(pUploadBufferData+sizeof(planeVertices)+sizeof(planeIndices),cubeVertices,sizeof(cubeVertices));
    memcpy(pUploadBufferData+sizeof(planeVertices)+sizeof(planeIndices)+sizeof(cubeVertices),cubeIndicies,sizeof(cubeIndicies));
    uploadBuffer->Unmap( 0, nullptr );

	streamingCommandList->CopyResource( defaultBuffer, uploadBuffer );

    //does this apply in my case https://twitter.com/MyNameIsMJP/status/1574431011579928580 ?
    planeVertexBufferView.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
    planeVertexBufferView.StrideInBytes = 3*sizeof(f32) + 3*sizeof(f32) + 4*sizeof(f32); //size of s single vertex
    planeVertexBufferView.SizeInBytes = sizeof(planeVertices);

	planeIndexBufferView.BufferLocation = planeVertexBufferView.BufferLocation + sizeof(planeVertices);
    planeIndexBufferView.SizeInBytes = sizeof(planeIndices);
    planeIndexBufferView.Format = DXGI_FORMAT_R32_UINT; 

    cubeVertexBufferView.BufferLocation = planeIndexBufferView.BufferLocation+sizeof(planeIndices);
    cubeVertexBufferView.StrideInBytes = 3*sizeof(f32) + 3*sizeof(f32) + 4*sizeof(f32); //size of s single vertex
    cubeVertexBufferView.SizeInBytes = sizeof(cubeVertices);

	cubeIndexBufferView.BufferLocation = cubeVertexBufferView.BufferLocation+sizeof(cubeVertices);
    cubeIndexBufferView.SizeInBytes = sizeof(cubeIndicies);
    cubeIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

inline
bool InitDirectX12()
{
//DXGI1_4 IDXGIFactory4, IDXGIAdapter3
//DXGI1_6 IDXGIFactory6, IDXGIFactory7, IDXGIAdapter4
	IDXGIFactory4* dxgiFactory;
#if MAIN_DEBUG
    u32 createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else
    u32 createFactoryFlags = 0;
#endif
	CreateDXGIFactory2( createFactoryFlags, IID_PPV_ARGS( &dxgiFactory ) );

	IDXGIAdapter1* adapter1 = nullptr;
	IDXGIAdapter3* adapter3 = nullptr;

	u64 amountOfVideoMemory = 0;
	for( u32 i = 0; dxgiFactory->EnumAdapters1( i, &adapter1 ) != DXGI_ERROR_NOT_FOUND; ++i )
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter1->GetDesc1( &desc );

		if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
		{
			adapter1->Release();
			continue;
		}
		if( SUCCEEDED( D3D12CreateDevice( adapter1, D3D_FEATURE_LEVEL_11_0, _uuidof( ID3D12Device ), nullptr ) ) )
		{
#if MAIN_DEBUG
			if( SUCCEEDED( adapter1->QueryInterface( IID_PPV_ARGS( &adapter3 ) ) ) )
			{
				assert( adapter1 == adapter3 );
				amountOfVideoMemory = desc.DedicatedVideoMemory;
				adapter1->Release();
				break;
			}
#else
			amountOfVideoMemory = desc.DedicatedVideoMemory;
			adapter3 = ( IDXGIAdapter3* )adapter1;
			break;
#endif
		}
		adapter1->Release();
	}
	dxgiFactory->Release();
	if( !adapter3 )
	{
		logError( "Error could not find a DirectX Adapter!\n" );  
		return false;
	}
#if MAIN_DEBUG
	printf( "Amount of Video Memory on selected D3D12 device: %lld\n", amountOfVideoMemory );
#endif
	u32 dwGPUNumber = 0; //0x1
	u32 dwVisibleGPUMask = 0;//0x1
	//actually retrieve the device interface to the adapter
	//IS D3D_FEATURE_LEVEL_12_0 DirectX12 or is D3D_FEATURE_LEVEL_11_0?
	if( FAILED( D3D12CreateDevice( adapter3, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS( &device ) ) ) )
	{
		logError( "Error could not open directx 12 supporting GPU!\n" );
		adapter3->Release();
		return false;
	}
	adapter3->Release();

#if MAIN_DEBUG
	//for getting errors from directx when debugging
	if( SUCCEEDED( device->QueryInterface( IID_PPV_ARGS( &pIQueue ) ) ) )
	{
		pIQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pIQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pIQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };
 
        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };
 
        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;
 
        if( FAILED( pIQueue->PushStorageFilter( &NewFilter ) ) )
        {
        	logError( "Detected device creation problem!\n" );  
        	return false;
        }
	}
#endif

	streamingQueue = InitCopyCommandQueue( device,dwGPUNumber );
	device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS( &streamingCommandAllocator[0] ) );
	device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS( &streamingCommandAllocator[1] ) );
	streamingFenceValue = 0;
	device->CreateFence( streamingFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &streamingFence ) );
	streamingFenceEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	device->CreateCommandList( dwGPUNumber, D3D12_COMMAND_LIST_TYPE_COPY, streamingCommandAllocator[0], NULL, IID_PPV_ARGS( &streamingCommandList ) );
	UploadModels(dwGPUNumber,dwVisibleGPUMask);
	streamingCommandList->Close();
	ID3D12CommandList* ppStreamingCommandLists[] = { streamingCommandList };
    streamingQueue->ExecuteCommandLists( _countof( ppStreamingCommandLists ), ppStreamingCommandLists );
	streamingQueue->Signal( streamingFence, ++streamingFenceValue );
	computeFenceValue = 0;
	device->CreateFence( computeFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &computeFence ) );
	streamingQueue->Wait( computeFence, 1 );

	const u64 qwComputeOutputDataSize = sizeof(ModelOutData);

	D3D12_RESOURCE_DESC computeOutputRsrcBufferDesc; //describes what is placed in heap
  	computeOutputRsrcBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  	computeOutputRsrcBufferDesc.Alignment = 0;
  	computeOutputRsrcBufferDesc.Width = qwComputeOutputDataSize;
  	computeOutputRsrcBufferDesc.Height = 1;
  	computeOutputRsrcBufferDesc.DepthOrArraySize = 1;
  	computeOutputRsrcBufferDesc.MipLevels = 1;
  	computeOutputRsrcBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
  	computeOutputRsrcBufferDesc.SampleDesc.Count = 1;
  	computeOutputRsrcBufferDesc.SampleDesc.Quality = 0;
  	computeOutputRsrcBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  	computeOutputRsrcBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; //D3D12_RESOURCE_FLAG_NONE 

	D3D12_RESOURCE_ALLOCATION_INFO computeAllocInfo = device->GetResourceAllocationInfo( dwVisibleGPUMask, 1, &computeOutputRsrcBufferDesc );
	u64 qwNumFullAlignments = qwComputeOutputDataSize / computeAllocInfo.Alignment;
	u64 qwExtraAlloc = qwComputeOutputDataSize % computeAllocInfo.Alignment;
	const u64 qwAlignedComputeOutputSize = (qwNumFullAlignments * computeAllocInfo.Alignment) + (qwExtraAlloc > 0 ? computeAllocInfo.Alignment : 0);
	const u64 qwComputeOutputHeapSize = qwAlignedComputeOutputSize * 2;

	D3D12_HEAP_DESC computeOutputHeapDesc;
	computeOutputHeapDesc.SizeInBytes = qwComputeOutputHeapSize;
	computeOutputHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	computeOutputHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	computeOutputHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	computeOutputHeapDesc.Properties.CreationNodeMask = dwGPUNumber;
	computeOutputHeapDesc.Properties.VisibleNodeMask = dwVisibleGPUMask;
	computeOutputHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; //64KB heap alignment, SizeInBytes should be a multiple of the heap alignment. is 64KB here 65536
	computeOutputHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

	device->CreateHeap( &computeOutputHeapDesc, IID_PPV_ARGS(&pComputeOutputHeap) );
#if MAIN_DEBUG
	pComputeOutputHeap->SetName( L"Compute Output Resource Heap" );
#endif
	device->CreatePlacedResource( pComputeOutputHeap,                          0, &computeOutputRsrcBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&computeOutputBuffer[0]) );
	device->CreatePlacedResource( pComputeOutputHeap, qwAlignedComputeOutputSize, &computeOutputRsrcBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&computeOutputBuffer[1]) );


	const u64 qwReadbackDataSize = sizeof(ModelOutData);

	D3D12_RESOURCE_DESC readbackRsrcBufferDesc; //describes what is placed in heap
  	readbackRsrcBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  	readbackRsrcBufferDesc.Alignment = 0;
  	readbackRsrcBufferDesc.Width = qwReadbackDataSize;
  	readbackRsrcBufferDesc.Height = 1;
  	readbackRsrcBufferDesc.DepthOrArraySize = 1;
  	readbackRsrcBufferDesc.MipLevels = 1;
  	readbackRsrcBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
  	readbackRsrcBufferDesc.SampleDesc.Count = 1;
  	readbackRsrcBufferDesc.SampleDesc.Quality = 0;
  	readbackRsrcBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  	readbackRsrcBufferDesc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; //D3D12_RESOURCE_FLAG_NONE 

	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo( dwVisibleGPUMask, 1, &readbackRsrcBufferDesc );
	qwNumFullAlignments = qwReadbackDataSize / allocInfo.Alignment;
	qwExtraAlloc = qwReadbackDataSize % allocInfo.Alignment;
	const u64 qwAlignedReadbackSize = (qwNumFullAlignments * allocInfo.Alignment) + (qwExtraAlloc > 0 ? allocInfo.Alignment : 0);
	const u64 qwReadbackHeapSize = qwAlignedReadbackSize * 2;

	D3D12_HEAP_DESC readbackHeapDesc;
	readbackHeapDesc.SizeInBytes = qwReadbackHeapSize;
	readbackHeapDesc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
	readbackHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	readbackHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	readbackHeapDesc.Properties.CreationNodeMask = dwGPUNumber;
	readbackHeapDesc.Properties.VisibleNodeMask = dwVisibleGPUMask;
	readbackHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; //64KB heap alignment, SizeInBytes should be a multiple of the heap alignment. is 64KB here 65536
	readbackHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

	device->CreateHeap( &readbackHeapDesc, IID_PPV_ARGS(&pReadbackHeap) );
#if MAIN_DEBUG
	pReadbackHeap->SetName( L"Readback Resource Heap" );
#endif
	device->CreatePlacedResource( pReadbackHeap,                     0, &readbackRsrcBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuffer[0]) );
	device->CreatePlacedResource( pReadbackHeap, qwAlignedReadbackSize, &readbackRsrcBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuffer[1]) );

    streamingCommandList->Reset(streamingCommandAllocator[1],NULL);
	streamingCommandList->CopyResource(readbackBuffer[0],computeOutputBuffer[0]);
	D3D12_RESOURCE_BARRIER readbackTransferToReadbackReadBarrier; //TODO does the readback buffer need to be in the source state to map and readback?
    readbackTransferToReadbackReadBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    readbackTransferToReadbackReadBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    readbackTransferToReadbackReadBarrier.Transition.pResource = readbackBuffer[0];
   	readbackTransferToReadbackReadBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    readbackTransferToReadbackReadBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    readbackTransferToReadbackReadBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    streamingCommandList->ResourceBarrier( 1, &readbackTransferToReadbackReadBarrier );
    streamingCommandList->Close();
	streamingQueue->ExecuteCommandLists( _countof( ppStreamingCommandLists ), ppStreamingCommandLists );
    streamingQueue->Signal( streamingFence, ++streamingFenceValue );


	if( FAILED( device->CreateRootSignature(dwGPUNumber, computeShaderBlob, sizeof(computeShaderBlob), IID_PPV_ARGS( &computeRootSignature ) ) ) )
	{
		logError( "Failed to create root signature!\n" );
		return false;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc;
	computePipelineStateDesc.pRootSignature = computeRootSignature;
	computePipelineStateDesc.CS.pShaderBytecode = computeShaderBlob;
	computePipelineStateDesc.CS.BytecodeLength = sizeof(computeShaderBlob);
	computePipelineStateDesc.NodeMask = dwGPUNumber;
	computePipelineStateDesc.CachedPSO.pCachedBlob = NULL;
	computePipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	device->CreateComputePipelineState ( &computePipelineStateDesc, IID_PPV_ARGS( &computePipelineStateObject ) );


    //Create Compute pipeline
	computeQueue = InitComputeCommandQueue( device, dwGPUNumber );
	computeQueue->Wait( streamingFence, 1 ); 
	device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS( &computeCommandAllocator[0] ) );
	device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS( &computeCommandAllocator[1] ) );
	computeFenceEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	device->CreateCommandList( dwGPUNumber, D3D12_COMMAND_LIST_TYPE_COMPUTE , computeCommandAllocator[0], computePipelineStateObject, IID_PPV_ARGS( &computeCommandList ) );
	

	D3D12_RESOURCE_BARRIER defaultHeapUploadToReadBarrier;
    defaultHeapUploadToReadBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    defaultHeapUploadToReadBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    defaultHeapUploadToReadBarrier.Transition.pResource = defaultBuffer;
   	defaultHeapUploadToReadBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    defaultHeapUploadToReadBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    defaultHeapUploadToReadBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; 
    computeCommandList->ResourceBarrier( 1, &defaultHeapUploadToReadBarrier );
	ComputeShaderCB cbValue;
	cbValue.dwOffsetsAndStrides0[0] = 0;
	cbValue.dwOffsetsAndStrides0[1] = 1;
	cbValue.dwOffsetsAndStrides0[2] = 2;
	cbValue.dwOffsetsAndStrides0[3] = 3;
	computeCommandList->SetComputeRootSignature( computeRootSignature ); //is this set with the pso?
	computeCommandList->SetComputeRoot32BitConstants(0,sizeof(ComputeShaderCB)/sizeof(u32),&cbValue,0);
	computeCommandList->SetComputeRootShaderResourceView(1,defaultBuffer->GetGPUVirtualAddress());
	computeCommandList->SetComputeRootUnorderedAccessView(2,computeOutputBuffer[0]->GetGPUVirtualAddress());
	computeCommandList->Dispatch(1,1,1);
	D3D12_RESOURCE_BARRIER computeOutputToComputeReadBarrier;
    computeOutputToComputeReadBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    computeOutputToComputeReadBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    computeOutputToComputeReadBarrier.Transition.pResource = computeOutputBuffer[0];
   	computeOutputToComputeReadBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    computeOutputToComputeReadBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    computeOutputToComputeReadBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    computeCommandList->ResourceBarrier( 1, &computeOutputToComputeReadBarrier );
    computeCommandList->Close();

    ID3D12CommandList* ppComputeCommandLists[] = { computeCommandList };
	computeQueue->ExecuteCommandLists( _countof( ppComputeCommandLists ), ppComputeCommandLists );
	//Stick at the end of the queue so we know when Our list will be ready
	computeQueue->Signal( computeFence, ++computeFenceValue ); 

	//this could have easly been done within the same command list execute as above but then we lose out on the signal, to start the rightback copies early. what is more efficent?
	computeCommandList->Reset(computeCommandAllocator[1],computePipelineStateObject);
	cbValue.dwOffsetsAndStrides0[0] = 4;
	cbValue.dwOffsetsAndStrides0[1] = 5;
	cbValue.dwOffsetsAndStrides0[2] = 6;
	cbValue.dwOffsetsAndStrides0[3] = 7;
	computeCommandList->SetComputeRootSignature( computeRootSignature ); 
	computeCommandList->SetComputeRoot32BitConstants(0,sizeof(ComputeShaderCB)/sizeof(u32),&cbValue,0);
	computeCommandList->SetComputeRootShaderResourceView(1,defaultBuffer->GetGPUVirtualAddress());
	computeCommandList->SetComputeRootUnorderedAccessView(2,computeOutputBuffer[1]->GetGPUVirtualAddress());
	computeCommandList->Dispatch(1,1,1);
	computeOutputToComputeReadBarrier.Transition.pResource = computeOutputBuffer[1];
	computeCommandList->ResourceBarrier( 1, &computeOutputToComputeReadBarrier );
	computeCommandList->Close();
	computeQueue->ExecuteCommandLists( _countof( ppComputeCommandLists ), ppComputeCommandLists );
	//Stick at the end of the queue so we know when Our list will be ready
	computeQueue->Signal( computeFence, ++computeFenceValue ); //we will not wait on the compute queue since the streaming queue will wait on it

	if( streamingFence->GetCompletedValue() < 1 )
    {
    	streamingFence->SetEventOnCompletion( 1, streamingFenceEvent );
		WaitForSingleObject( streamingFenceEvent, INFINITE );
    }

    streamingCommandList->Reset(streamingCommandAllocator[0],NULL);
    streamingQueue->Wait( computeFence, 2 );
	streamingCommandList->CopyResource(readbackBuffer[1],computeOutputBuffer[1]);
	readbackTransferToReadbackReadBarrier.Transition.pResource = readbackBuffer[1];
	streamingCommandList->ResourceBarrier( 1, &readbackTransferToReadbackReadBarrier );
	streamingCommandList->Close();
	streamingQueue->ExecuteCommandLists( _countof( ppStreamingCommandLists ), ppStreamingCommandLists );
    streamingQueue->Signal( streamingFence, ++streamingFenceValue );

    //first readback is ready
    if( streamingFence->GetCompletedValue() < 2 )
    {
    	streamingFence->SetEventOnCompletion( 2, streamingFenceEvent );
		WaitForSingleObject( streamingFenceEvent, INFINITE );
    }

    ModelOutData readbackData[2];
    u8* pOutputDataBufferData;
    if( FAILED( readbackBuffer[0]->Map( 0, nullptr, (void**) &pOutputDataBufferData ) ) )
    {
        return false;
    }
    memcpy(&readbackData[0],pOutputDataBufferData,sizeof(ModelOutData));
    D3D12_RANGE emptyRange;
    emptyRange.Begin = 0;
    emptyRange.End = 0;
    readbackBuffer[0]->Unmap( 0, &emptyRange ); //signal we didn't write anything

    //second readback is ready
    if( streamingFence->GetCompletedValue() < 3 )
    {
    	streamingFence->SetEventOnCompletion( 3, streamingFenceEvent );
		WaitForSingleObject( streamingFenceEvent, INFINITE );
    }

    if( FAILED( readbackBuffer[1]->Map( 0, nullptr, (void**) &pOutputDataBufferData ) ) )
    {
        return false;
    }
    memcpy(&readbackData[1],pOutputDataBufferData,sizeof(ModelOutData));
    readbackBuffer[1]->Unmap( 0, &emptyRange ); //signal we didn't write anything

    printf("%u %u %u %u\n%u %u %u %u\n",readbackData[0].dwData[0],readbackData[0].dwData[1],readbackData[0].dwData[2],readbackData[0].dwData[3],
    									readbackData[1].dwData[0],readbackData[1].dwData[1],readbackData[1].dwData[2],readbackData[1].dwData[3]);

	return true;
}

int main()
{
	SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );

#if MAIN_DEBUG
	if( !EnableDebugLayer() )
	{
		return -1;
	}
#endif


	if( !InitDirectX12() )
	{
		return -1;
	}

/*

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency( &PerfCountFrequencyResult );
    int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    LARGE_INTEGER LastCounter;
    QueryPerformanceCounter( &LastCounter );

    while( Running )
    {
    	//synchronization
        UpdateCurrentFrame();
        if( !WaitCurrentFrame() )
        {
			continue;
        }

    	uint64_t EndCycleCount = __rdtsc();
    
    	LARGE_INTEGER EndCounter;
    	QueryPerformanceCounter(&EndCounter);
    
    	//Display the value here
    	s64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
    	f32 deltaTime = CounterElapsed / (f32)PerfCountFrequency ;
    	f64 MSPerFrame = (f64) ( ( 1000.0f * CounterElapsed ) / (f64)PerfCountFrequency );
    	f64 FPS = PerfCountFrequency / (f64)CounterElapsed;
    	LastCounter = EndCounter;

    	//render empty frame while minimized to make power usage go down to very low while minimized (and vsync it)
    	if( !SignalCurrentFrame() )
    	{
    		return -1;
    	}
    }
*/
  
 //going to clean up only in Debug mode (so we know exactly what is allocated on close), so we aren't wasting the user's time in actual release on close 
#if MAIN_DEBUG
	/*
	FlushCommandQueue();
	rtvDescriptorHeap->Release();
	dsDescriptorHeap->Release();
	planeVertexBuffer->Release();
	planeIndexBuffer->Release(); 
	cubeVertexBuffer->Release(); 
	cubeIndexBuffer->Release();
	commandList->Release();
	for( u32 dwFrame = 0; dwFrame < NUM_FRAMES; ++dwFrame )
    {
    	backBufferRenderTargets[dwFrame]->Release();
    	commandAllocator[dwFrame]->Release();
    	fences[dwFrame]->Release();
    }
    depthStencilBuffer->Release();
    commandQueue->Release();
	//TODO remove these when we can release them ealier
	planeVertexUploadBuffer->Release();
	planeIndexUploadBuffer->Release();
	cubeVertexUploadBuffer->Release();
	cubeIndexUploadBuffer->Release(); 

	pipelineStateObject->Release();
	rootSignature->Release();
    CloseHandle(fenceEvent);
    swapChain->Release();
    //Is this ok todo with debug enabled by these?
	debugInterface->Release(); 
	pIQueue->Release(); 

    device->Release();


	//CoUninitialize();
    atexit(&ReportLiveObjects);
    */
#endif
    return 0;
}
