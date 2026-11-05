#include "Renderer.hpp"
#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/video.h>
#include <malloc.h>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <ogc/system.h>
#include <ogc/conf.h>

#define DEFAULT_FIFO_SIZE (256 * 1024)

namespace Renderer {

static GXRModeObj* s_vmode = nullptr;
static GXRModeObj s_videoMode;
static void* s_xfb[2] = { nullptr, nullptr };
static int s_fbIndex = 0;
static void* s_fifo = nullptr;

static Camera s_camera;
static float s_cameraCos = 1.0f, s_cameraSin = 0.0f;
static int s_screenWidth = 640;
static int s_screenHeight = 480;

static uint8_t s_bgR = 230, s_bgG = 245, s_bgB = 255;

// Basic 8x8 font bitmap table for ASCII 32..127
static const uint8_t FONT_8X8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // 35 '#'
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // 36 '$'
    {0x00,0x66,0xA6,0xD4,0x2B,0x65,0x66,0x00}, // 37 '%'
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 38 '&'
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // 47 '/'
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 48 '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 '1'
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // 50 '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 51 '3'
    {0x0C,0x1C,0x34,0x64,0x7E,0x04,0x04,0x00}, // 52 '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 53 '5'
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 54 '6'
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // 55 '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 56 '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // 57 '9'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 58 ':'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // 59 ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 '<'
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 '='
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 '>'
    {0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00}, // 63 '?'
    {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00}, // 64 '@'
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 65 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 66 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 67 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 68 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 69 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 70 'F'
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // 71 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 72 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 73 'I'
    {0x0E,0x06,0x06,0x06,0x06,0x66,0x3C,0x00}, // 74 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 75 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 76 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 78 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 79 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 80 'P'
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // 81 'Q'
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 82 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 83 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 85 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 86 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 88 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 89 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 90 'Z'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 '['
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // 92 '\'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ']'
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00}, // 95 '_'
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 97 'a'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 'b'
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 99 'c'
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 'd'
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 'e'
    {0x0E,0x18,0x7E,0x18,0x18,0x18,0x18,0x00}, // 102 'f'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C}, // 103 'g'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 104 'h'
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 'i'
    {0x06,0x00,0x0E,0x06,0x06,0x66,0x3C,0x00}, // 106 'j'
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 107 'k'
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 'l'
    {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00}, // 109 'm'
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 110 'n'
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 'o'
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 112 'p'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // 113 'q'
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 114 'r'
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 115 's'
    {0x18,0x18,0x7E,0x18,0x18,0x18,0x0E,0x00}, // 116 't'
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 'u'
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 'v'
    {0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00}, // 119 'w'
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 'x'
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x7C}, // 121 'y'
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 'z'
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 '{'
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 '|'
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 '}'
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}  // 126 '~'
};

bool Init() {
    VIDEO_Init();

    const GXRModeObj* preferredMode = VIDEO_GetPreferredMode(nullptr);
    if (!preferredMode) return false;
    s_videoMode = *preferredMode;
    s_vmode = &s_videoMode;

    // Use the entire 720-sample video line. GX renders to its native EFB;
    // the VI scales that framebuffer horizontally to the full output width.
    s_vmode->viWidth = VI_MAX_WIDTH_NTSC;
    s_vmode->viXOrigin = 0;

    s_screenHeight = s_vmode->efbHeight;
    // Logical coordinates follow the TV aspect, keeping eyes and blobs round
    // on both 4:3 and anamorphic 16:9 output.
    float aspect = CONF_GetAspectRatio() == CONF_ASPECT_16_9 ? 16.0f / 9.0f : 4.0f / 3.0f;
    s_screenWidth = static_cast<int>(std::lround(s_screenHeight * aspect));

    s_xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_vmode));
    s_xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_vmode));

    VIDEO_Configure(s_vmode);
    VIDEO_SetNextFramebuffer(s_xfb[s_fbIndex]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (s_vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    s_fifo = memalign(32, DEFAULT_FIFO_SIZE);
    memset(s_fifo, 0, DEFAULT_FIFO_SIZE);

    GX_Init(s_fifo, DEFAULT_FIFO_SIZE);

    GXColor background = { s_bgR, s_bgG, s_bgB, 0xFF };
    GX_SetCopyClear(background, 0x00FFFFFF);

    GX_SetViewport(0, 0, s_vmode->fbWidth, s_vmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)s_vmode->xfbHeight / (f32)s_vmode->efbHeight);
    GX_SetScissor(0, 0, s_vmode->fbWidth, s_vmode->efbHeight);
    GX_SetDispCopySrc(0, 0, s_vmode->fbWidth, s_vmode->efbHeight);
    GX_SetDispCopyDst(s_vmode->fbWidth, s_vmode->xfbHeight);
    GX_SetCopyFilter(s_vmode->aa, s_vmode->sample_pattern, GX_TRUE, s_vmode->vfilter);
    GX_SetFieldMode(s_vmode->field_rendering, ((s_vmode->viHeight == 2 * s_vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(s_xfb[s_fbIndex], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // Setup 2D Orthographic Projection
    Mtx44 proj;
    guOrtho(proj, 0, s_screenHeight, 0, s_screenWidth, 0.1f, 300.0f);
    GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);

    // Setup Vertex Descriptors
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    // Alpha blending
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_DISABLE);

    return true;
}

void SetCamera(const Camera& cam) {
    s_camera = cam;
    s_cameraCos = std::cos(cam.angle);
    s_cameraSin = std::sin(cam.angle);
}

const Camera& GetCamera() {
    return s_camera;
}

int GetScreenWidth() { return s_screenWidth; }
int GetScreenHeight() { return s_screenHeight; }

void ClearBackground(uint8_t r, uint8_t g, uint8_t b) {
    s_bgR = r; s_bgG = g; s_bgB = b;
    GXColor bg = { r, g, b, 0xFF };
    GX_SetCopyClear(bg, 0x00FFFFFF);
}

void BeginFrame() {
    Mtx modelview;
    guMtxIdentity(modelview);
    guMtxTransApply(modelview, modelview, 0.0f, 0.0f, -50.0f);
    GX_LoadPosMtxImm(modelview, GX_PNMTX0);

    GX_InvVtxCache();
    GX_InvalidateTexAll();
}

void EndFrame() {
    GX_DrawDone();

    s_fbIndex ^= 1;
    GX_CopyDisp(s_xfb[s_fbIndex], GX_TRUE);
    GX_Flush();

    VIDEO_SetNextFramebuffer(s_xfb[s_fbIndex]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

// Transform world coordinates to screen viewport based on camera
static inline void WorldToScreen(float wx, float wy, float& sx, float& sy) {
    // 1. Translate relative to camera center
    float dx = wx - s_camera.x;
    float dy = wy - s_camera.y;

    // 2. Rotate by camera tilt angle (matches PSP screen tilt)
    float cosA = s_cameraCos;
    float sinA = s_cameraSin;
    float rx = dx * cosA - dy * sinA;
    float ry = dx * sinA + dy * cosA;

    // 3. Scale and center on screen
    sx = (s_screenWidth * 0.5f) + rx * s_camera.scale;
    sy = (s_screenHeight * 0.5f) + ry * s_camera.scale;
}

void DrawWorldTriangle(const LocoData::Vec2& a, const LocoData::Vec2& b, const LocoData::Vec2& c, LocoData::Color4 color) {
    float ax, ay, bx, by, cx, cy;
    WorldToScreen(a.x, a.y, ax, ay);
    WorldToScreen(b.x, b.y, bx, by);
    WorldToScreen(c.x, c.y, cx, cy);

    // Simple culling check
    float minX = std::min(ax, std::min(bx, cx));
    float maxX = std::max(ax, std::max(bx, cx));
    float minY = std::min(ay, std::min(by, cy));
    float maxY = std::max(ay, std::max(by, cy));

    if (maxX < -50.0f || minX > s_screenWidth + 50.0f || maxY < -50.0f || minY > s_screenHeight + 50.0f) {
        return;
    }

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GX_Position3f32(ax, ay, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    GX_Position3f32(bx, by, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    GX_Position3f32(cx, cy, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_End();
}

void DrawWorldMesh(const LocoData::PolyMesh& mesh, float offsetX, float offsetY, float scale, float angle) {
    if (mesh.numTris <= 0) return;

    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    // Mesh geometry is immutable. Cache its enclosing circle once, then reject
    // only objects wholly outside the view, including during camera rotation.
    struct Bounds { float x,y,r; };
    static std::unordered_map<const LocoData::Tri2*,Bounds> bounds;
    Bounds temporary{};
    auto found=mesh.numOutline>0?bounds.find(mesh.triangles):bounds.end();
    if(found==bounds.end()) {
        float minX=mesh.triangles[0].a.x,maxX=minX;
        float minY=mesh.triangles[0].a.y,maxY=minY;
        for(int i=0;i<mesh.numTris;++i)
            for(auto p:{mesh.triangles[i].a,mesh.triangles[i].b,mesh.triangles[i].c}) {
                minX=std::min(minX,p.x);maxX=std::max(maxX,p.x);
                minY=std::min(minY,p.y);maxY=std::max(maxY,p.y);
            }
        temporary={(minX+maxX)*.5f,(minY+maxY)*.5f,std::hypot(maxX-minX,maxY-minY)*.5f};
        if(mesh.numOutline>0)found=bounds.emplace(mesh.triangles,temporary).first;
    }
    const auto& b=found==bounds.end()?temporary:found->second;
    float sx,sy;
    WorldToScreen(offsetX+scale*(b.x*cosA-b.y*sinA),offsetY+scale*(b.x*sinA+b.y*cosA),sx,sy);
    float radius=b.r*std::abs(scale*s_camera.scale)+2.0f;
    if(sx+radius<0 || sy+radius<0 || sx-radius>s_screenWidth || sy-radius>s_screenHeight) return;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, mesh.numTris * 3);
    for (int i = 0; i < mesh.numTris; ++i) {
        const auto& t = mesh.triangles[i];

        auto streamPoint = [&](const LocoData::Vec2& p) {
            float px = p.x * scale;
            float py = p.y * scale;
            float rx = offsetX + px * cosA - py * sinA;
            float ry = offsetY + px * sinA + py * cosA;
            float sx, sy;
            WorldToScreen(rx, ry, sx, sy);
            GX_Position3f32(sx, sy, 0.0f);
            GX_Color4u8(mesh.color.r, mesh.color.g, mesh.color.b, mesh.color.a);
        };

        streamPoint(t.a);
        streamPoint(t.b);
        streamPoint(t.c);
    }
    GX_End();
}

void DrawWorldMeshRotatedAround(const LocoData::PolyMesh& mesh, float pivotX, float pivotY, float angle) {
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    DrawWorldMesh(mesh,pivotX-pivotX*cosA+pivotY*sinA,
                  pivotY-pivotX*sinA-pivotY*cosA,1.0f,angle);
}

void DrawWorldCircle(float cx, float cy, float radius, LocoData::Color4 color, int segments) {
    if (segments < 6) segments = 6;
    float scx, scy;
    WorldToScreen(cx, cy, scx, scy);
    float srad = radius * s_camera.scale;

    if (scx + srad < 0 || scx - srad > s_screenWidth || scy + srad < 0 || scy - srad > s_screenHeight) {
        return;
    }

    GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
    GX_Position3f32(scx, scy, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    for (int i = 0; i <= segments; ++i) {
        float theta = (i * 2.0f * M_PI) / segments;
        float px = scx + std::cos(theta) * srad;
        float py = scy + std::sin(theta) * srad;
        GX_Position3f32(px, py, 0.0f);
        GX_Color4u8(color.r, color.g, color.b, color.a);
    }
    GX_End();
}

void DrawWorldBlob(const std::vector<LocoData::Vec2>& rawPoints, LocoData::Color4 fillColor, LocoData::Color4 outlineColor) {
    size_t n = rawPoints.size();
    if (n < 3) return;

    // Loco already supplies a rounded offset curve (7 samples per skin node).
    // Only legacy sparse polygons need another smoothing pass.
    std::vector<LocoData::Vec2> current = rawPoints;
    for (int iter = 0; iter < (n < 48 ? 2 : 0); ++iter) {
        std::vector<LocoData::Vec2> next;
        next.reserve(current.size() * 2);
        size_t count = current.size();
        for (size_t i = 0; i < count; ++i) {
            const auto& p0 = current[i];
            const auto& p1 = current[(i + 1) % count];
            next.push_back({ 0.75f * p0.x + 0.25f * p1.x, 0.75f * p0.y + 0.25f * p1.y });
            next.push_back({ 0.25f * p0.x + 0.75f * p1.x, 0.25f * p0.y + 0.75f * p1.y });
        }
        current = std::move(next);
    }

    size_t smoothCount = current.size();

    // Calculate center
    float avgX = 0, avgY = 0;
    for (size_t i = 0; i < smoothCount; ++i) {
        avgX += current[i].x;
        avgY += current[i].y;
    }
    avgX /= smoothCount;
    avgY /= smoothCount;

    float scx, scy;
    WorldToScreen(avgX, avgY, scx, scy);

    // Draw solid body with triangle fan
    GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, smoothCount + 2);
    GX_Position3f32(scx, scy, 0.0f);
    GX_Color4u8(fillColor.r, fillColor.g, fillColor.b, fillColor.a);

    for (size_t i = 0; i <= smoothCount; ++i) {
        size_t idx = i % smoothCount;
        float px, py;
        WorldToScreen(current[idx].x, current[idx].y, px, py);
        GX_Position3f32(px, py, 0.0f);
        GX_Color4u8(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    }
    GX_End();

    // Draw smooth outline
    if (outlineColor.r == fillColor.r && outlineColor.g == fillColor.g &&
        outlineColor.b == fillColor.b && outlineColor.a == fillColor.a) return;
    GX_SetLineWidth(static_cast<u8>(std::max(1.0f, 2.5f * s_camera.scale)), GX_TO_ZERO);
    GX_Begin(GX_LINESTRIP, GX_VTXFMT0, smoothCount + 1);
    for (size_t i = 0; i <= smoothCount; ++i) {
        size_t idx = i % smoothCount;
        float px, py;
        WorldToScreen(current[idx].x, current[idx].y, px, py);
        GX_Position3f32(px, py, 0.0f);
        GX_Color4u8(outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    }
    GX_End();
}

void DrawWorldLine(const LocoData::Vec2& a, const LocoData::Vec2& b, float width, LocoData::Color4 color) {
    float ax, ay, bx, by;
    WorldToScreen(a.x, a.y, ax, ay);
    WorldToScreen(b.x, b.y, bx, by);

    GX_SetLineWidth(static_cast<u8>(std::clamp(width * s_camera.scale * 6.0f, 1.0f, 255.0f)), GX_TO_ZERO);
    GX_Begin(GX_LINES, GX_VTXFMT0, 2);
    GX_Position3f32(ax, ay, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    GX_Position3f32(bx, by, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_End();
}

static void DrawSprite(const Sprite& sprite, float cx, float cy, float width, float height,
                     LocoData::Color4 tint, float angle,bool screen) {
    if(!screen) {
        float x,y;WorldToScreen(cx,cy,x,y);
        float radius=std::hypot(width,height)*.5f*std::abs(s_camera.scale)+2.0f;
        if(x+radius<0 || y+radius<0 || x-radius>s_screenWidth || y-radius>s_screenHeight) return;
    }
    static const uint8_t* flushed[64] = {};
    static int numFlushed = 0;
    bool known = false;
    for (int i = 0; i < numFlushed; ++i) known = known || flushed[i] == sprite.pixels;
    if (!known) {
        DCFlushRange(const_cast<uint8_t*>(sprite.pixels), sprite.width * sprite.height * 4);
        if (numFlushed < 64) flushed[numFlushed++] = sprite.pixels;
    }
    GXTexObj texture;
    GX_InitTexObj(&texture, const_cast<uint8_t*>(sprite.pixels), sprite.width, sprite.height,
                  GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjLOD(&texture, GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GX_LoadTexObj(&texture, GX_TEXMAP0);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    const float cosine = std::cos(angle), sine = std::sin(angle);
    const float corners[4][2] = {{-0.5f,-0.5f},{0.5f,-0.5f},{0.5f,0.5f},{-0.5f,0.5f}};
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    for (const auto& corner : corners) {
        float x = corner[0] * width, y = corner[1] * height, sx, sy;
        sx=cx+cosine*x-sine*y;sy=cy+sine*x+cosine*y;
        if(!screen) WorldToScreen(sx,sy,sx,sy);
        GX_Position3f32(sx, sy, 0);
        GX_Color4u8(tint.r, tint.g, tint.b, tint.a);
        GX_TexCoord2f32(corner[0] + 0.5f, corner[1] + 0.5f);
    }
    GX_End();
    GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
}

void DrawWorldTexturedTriangles(const Sprite& sprite,const TexturedVertex* vertices,int count,bool repeat) {
    if(count<=0 || count>65535)return;
    GXTexObj texture;
    GX_InitTexObj(&texture,const_cast<uint8_t*>(sprite.pixels),sprite.width,sprite.height,GX_TF_RGBA8,repeat?GX_REPEAT:GX_CLAMP,repeat?GX_REPEAT:GX_CLAMP,GX_FALSE);
    GX_InitTexObjLOD(&texture,GX_LINEAR,GX_LINEAR,0,0,0,GX_FALSE,GX_FALSE,GX_ANISO_1);
    GX_LoadTexObj(&texture,GX_TEXMAP0);GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
    GX_SetVtxDesc(GX_VA_TEX0,GX_DIRECT);GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
    GX_Begin(GX_TRIANGLES,GX_VTXFMT0,count);
    for(int i=0;i<count;++i){const auto& v=vertices[i];float x,y;WorldToScreen(v.x,v.y,x,y);
        GX_Position3f32(x,y,0);GX_Color4u8(v.r,v.g,v.b,v.a);GX_TexCoord2f32(v.u,v.v);}
    GX_End();GX_SetVtxDesc(GX_VA_TEX0,GX_NONE);GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORDNULL,GX_TEXMAP_NULL,GX_COLOR0A0);GX_SetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
}

void DrawWorldSprite(const Sprite& sprite,float x,float y,float w,float h,LocoData::Color4 tint,float angle) {
    DrawSprite(sprite,x,y,w,h,tint,angle,false);
}
void DrawScreenSprite(const Sprite& sprite,float x,float y,float w,float h,LocoData::Color4 tint,float angle) {
    DrawSprite(sprite,x,y,w,h,tint,angle,true);
}
void DrawScreenEllipse(float x,float y,float rx,float ry,LocoData::Color4 color) {
    GX_Begin(GX_TRIANGLEFAN,GX_VTXFMT0,66);
    GX_Position3f32(x,y,0);GX_Color4u8(color.r,color.g,color.b,color.a);
    for(int i=0;i<=64;++i){float a=i*2.0f*M_PI/64;GX_Position3f32(x+rx*std::cos(a),y+ry*std::sin(a),0);GX_Color4u8(color.r,color.g,color.b,color.a);}
    GX_End();
}
void DrawScreenRect(float x, float y, float w, float h, LocoData::Color4 color) {
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(x, y, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    GX_Position3f32(x + w, y, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    GX_Position3f32(x + w, y + h, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    GX_Position3f32(x, y + h, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_End();
}

void DrawScreenCircle(float cx, float cy, float radius, LocoData::Color4 color, int segments) {
    GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
    GX_Position3f32(cx, cy, 0.0f);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    for (int i = 0; i <= segments; ++i) {
        float theta = (i * 2.0f * M_PI) / segments;
        float px = cx + std::cos(theta) * radius;
        float py = cy + std::sin(theta) * radius;
        GX_Position3f32(px, py, 0.0f);
        GX_Color4u8(color.r, color.g, color.b, color.a);
    }
    GX_End();
}

void DrawScreenText(float x, float y, const char* text, LocoData::Color4 color, float scale) {
    if (!text) return;
    float curX = x;
    float curY = y;

    int totalQuads = 0;
    for (const char* c = text; *c != '\0'; ++c) {
        if (*c == '\n') continue;
        uint8_t ascii = static_cast<uint8_t>(*c);
        if (ascii < 32 || ascii > 126) ascii = 32;
        const uint8_t* glyph = FONT_8X8[ascii - 32];
        for (int r = 0; r < 8; ++r) {
            uint8_t byte = glyph[r];
            for (int col = 0; col < 8; ++col) {
                if (byte & (0x80 >> col)) totalQuads++;
            }
        }
    }
    if (totalQuads == 0) return;

    GX_Begin(GX_QUADS, GX_VTXFMT0, totalQuads * 4);
    for (const char* c = text; *c != '\0'; ++c) {
        if (*c == '\n') {
            curX = x;
            curY += 10.0f * scale;
            continue;
        }

        uint8_t ascii = static_cast<uint8_t>(*c);
        if (ascii < 32 || ascii > 126) ascii = 32;
        const uint8_t* glyph = FONT_8X8[ascii - 32];

        for (int row = 0; row < 8; ++row) {
            uint8_t byte = glyph[row];
            for (int col = 0; col < 8; ++col) {
                if (byte & (0x80 >> col)) {
                    float px = curX + col * scale;
                    float py = curY + row * scale;
                    GX_Position3f32(px, py, 0.0f);
                    GX_Color4u8(color.r, color.g, color.b, color.a);
                    GX_Position3f32(px + scale, py, 0.0f);
                    GX_Color4u8(color.r, color.g, color.b, color.a);
                    GX_Position3f32(px + scale, py + scale, 0.0f);
                    GX_Color4u8(color.r, color.g, color.b, color.a);
                    GX_Position3f32(px, py + scale, 0.0f);
                    GX_Color4u8(color.r, color.g, color.b, color.a);
                }
            }
        }
        curX += 8.0f * scale;
    }
    GX_End();
}

} // namespace Renderer
