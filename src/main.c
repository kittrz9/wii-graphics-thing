#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <math.h>

#include "model.h"

#define FIFO_SIZE (256*1024)

static vu8 buffersReady;
static void* frameBuffer;

u8 vertColors[] ATTRIBUTE_ALIGN(32) = {
	255, 0, 0, 255,
	0, 255, 0, 255,
	0, 0, 255, 255,
};

static void copyBuffers(u32 unused);

int main(int argc, char** argv) {
	GXColor backgroundColor = {32, 32, 32, 255};
	void* fifoBuffer = NULL;
	VIDEO_Init();
	WPAD_Init();
	GXRModeObj* screenMode = VIDEO_GetPreferredMode(NULL);
	frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));

	VIDEO_Configure(screenMode);
	VIDEO_SetNextFramebuffer(frameBuffer);
	VIDEO_SetPostRetraceCallback(copyBuffers);
	VIDEO_SetBlack(0);
	VIDEO_Flush();

	fifoBuffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));
	memset(fifoBuffer, 0, FIFO_SIZE);

	GX_Init(fifoBuffer, FIFO_SIZE);
	GX_SetCopyClear(backgroundColor, GX_MAX_Z24);
	GX_SetViewport(0, 0, screenMode->fbWidth, screenMode->efbHeight, 0, 1);
	GX_SetDispCopyYScale((f32)screenMode->xfbHeight/(f32)screenMode->efbHeight);
	GX_SetScissor(0, 0, screenMode->fbWidth, screenMode->efbHeight);
	GX_SetDispCopySrc(0, 0, screenMode->fbWidth, screenMode->efbHeight);
	GX_SetDispCopyDst(screenMode->fbWidth, screenMode->xfbHeight);
	GX_SetCopyFilter(screenMode->aa, screenMode->sample_pattern, GX_TRUE, screenMode->vfilter);
	GX_SetFieldMode(screenMode->field_rendering, ((screenMode->viHeight==2*screenMode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	GX_SetCullMode(GX_CULL_FRONT);
	GX_CopyDisp(frameBuffer, GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	guVector camera = {0.0f, 0.0f, 0.0f};
	guVector up = {0.0f, 1.0f, 0.0f};
	guVector look = {0.0f, 0.0f, -1.0f};

	Mtx44 projection;
	guPerspective(projection, 60, 1.33f, 10.0f, 300.0f);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
	GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetArray(GX_VA_POS, vertPositions, 3*sizeof(f32));
	GX_SetArray(GX_VA_CLR0, vertColors, 4*sizeof(u8));
	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

	f32 angle = 0.0f;

	Mtx view;
	Mtx model;
	while(1) {
		angle += 0.1f;
		guLookAt(view, &camera, &up, &look);
		GX_SetViewport(0, 0, screenMode->fbWidth, screenMode->efbHeight, 0, 1);
		GX_InvVtxCache();
		GX_InvalidateTexAll();

		// why cant you rotate a matrix over multiple axises???
		Mtx rx;
		guMtxIdentity(model);
		guMtxRotRad(model, 'y', angle);
		guMtxApplyScale(model, model, 10.0f, 10.0f, 10.0f);
		guMtxIdentity(rx);
		guMtxRotDeg(rx, 'x', 45);
		guMtxConcat(model, rx, model);
		guMtxTransApply(view, view, 0.0f, 0.0f, -50.0f);
		guMtxConcat(view, model, model);

		GX_LoadPosMtxImm(model, GX_PNMTX0);

		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, indexCount);

		for(u32 i = 0; i < indexCount; ++i) {
			GX_Position1x16(vertIndices[i]);
			GX_Color1x8(i%3);
		}

		GX_End();

		GX_DrawDone();

		VIDEO_WaitVSync();

		buffersReady = GX_TRUE;

		VIDEO_WaitVSync();

		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(0);

		if(pressed & WPAD_BUTTON_HOME) { exit(0); };
		if(pressed & WPAD_BUTTON_2) { camera.z += 0.5f; }
		if(pressed & WPAD_BUTTON_1) { camera.z -= 0.5f; }
	}

	return 0;
}

static void copyBuffers(u32 unused) {
	if(buffersReady == GX_TRUE) {
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer, GX_TRUE);
		GX_Flush();
		buffersReady = GX_FALSE;
	}
}
