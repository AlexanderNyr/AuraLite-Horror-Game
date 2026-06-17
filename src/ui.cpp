#include "ui.hpp"
#include <vector>
#include <cmath>
#include <cstring>
#include <iostream>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// A compact, highly-readable retro 8x8 font for ASCII characters 32 to 126
// Each byte represents a row of 8 pixels (MSB is left pixel)
static const uint8_t font8x8_basic[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (32)
    {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00}, // '!'
    {0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00}, // '"'
    {0x24, 0x24, 0x7E, 0x24, 0x7E, 0x24, 0x24, 0x00}, // '#'
    {0x12, 0x3F, 0x48, 0x3E, 0x09, 0x7E, 0x24, 0x00}, // '$'
    {0x44, 0x4A, 0x14, 0x08, 0x28, 0x52, 0x22, 0x00}, // '%'
    {0x30, 0x4A, 0x4A, 0x34, 0x4D, 0x42, 0x3D, 0x00}, // '&'
    {0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, // '''
    {0x10, 0x20, 0x40, 0x40, 0x40, 0x20, 0x10, 0x00}, // '('
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08, 0x00}, // ')'
    {0x00, 0x24, 0x18, 0x3C, 0x18, 0x24, 0x00, 0x00}, // '*'
    {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00}, // '+'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08}, // ','
    {0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00}, // '-'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // '.'
    {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00}, // '/'
    {0x3E, 0x45, 0x49, 0x51, 0x61, 0x41, 0x3E, 0x00}, // '0' (48)
    {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00}, // '1'
    {0x3E, 0x41, 0x02, 0x0C, 0x30, 0x40, 0x7F, 0x00}, // '2'
    {0x3E, 0x41, 0x02, 0x1C, 0x02, 0x41, 0x3E, 0x00}, // '3'
    {0x04, 0x0C, 0x14, 0x24, 0x7F, 0x04, 0x04, 0x00}, // '4'
    {0x7F, 0x40, 0x40, 0x3E, 0x01, 0x01, 0x7E, 0x00}, // '5'
    {0x3E, 0x40, 0x40, 0x7E, 0x41, 0x41, 0x3E, 0x00}, // '6'
    {0x7F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10, 0x00}, // '7'
    {0x3E, 0x41, 0x41, 0x3E, 0x41, 0x41, 0x3E, 0x00}, // '8'
    {0x3E, 0x41, 0x41, 0x7F, 0x01, 0x01, 0x3E, 0x00}, // '9'
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, // ':'
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x08, 0x00}, // ';'
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x00}, // '<'
    {0x00, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x00, 0x00}, // '='
    {0x40, 0x20, 0x10, 0x08, 0x10, 0x20, 0x40, 0x00}, // '>'
    {0x3E, 0x41, 0x02, 0x0C, 0x18, 0x00, 0x18, 0x00}, // '?'
    {0x3E, 0x41, 0x5D, 0x55, 0x5D, 0x40, 0x3E, 0x00}, // '@'
    {0x1C, 0x22, 0x41, 0x41, 0x7F, 0x41, 0x41, 0x00}, // 'A' (65)
    {0x7E, 0x41, 0x41, 0x7E, 0x41, 0x41, 0x7E, 0x00}, // 'B'
    {0x3E, 0x41, 0x40, 0x40, 0x40, 0x41, 0x3E, 0x00}, // 'C'
    {0x7C, 0x42, 0x41, 0x41, 0x41, 0x42, 0x7C, 0x00}, // 'D'
    {0x7F, 0x40, 0x40, 0x7E, 0x40, 0x40, 0x7F, 0x00}, // 'E'
    {0x7F, 0x40, 0x40, 0x7E, 0x40, 0x40, 0x40, 0x00}, // 'F'
    {0x3E, 0x41, 0x40, 0x4F, 0x41, 0x41, 0x3E, 0x00}, // 'G'
    {0x41, 0x41, 0x41, 0x7F, 0x41, 0x41, 0x41, 0x00}, // 'H'
    {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00}, // 'I'
    {0x1F, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1C, 0x00}, // 'J'
    {0x41, 0x42, 0x44, 0x78, 0x44, 0x42, 0x41, 0x00}, // 'K'
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7F, 0x00}, // 'L'
    {0x41, 0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x00}, // 'M'
    {0x41, 0x51, 0x51, 0x49, 0x45, 0x45, 0x41, 0x00}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00}, // 'O'
    {0x7E, 0x41, 0x41, 0x7E, 0x40, 0x40, 0x40, 0x00}, // 'P'
    {0x3E, 0x41, 0x41, 0x41, 0x45, 0x42, 0x3D, 0x00}, // 'Q'
    {0x7E, 0x41, 0x41, 0x7E, 0x44, 0x42, 0x41, 0x00}, // 'R'
    {0x3E, 0x41, 0x40, 0x3E, 0x01, 0x41, 0x3E, 0x00}, // 'S'
    {0x7F, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // 'T'
    {0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00}, // 'U'
    {0x41, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x00}, // 'V'
    {0x41, 0x41, 0x41, 0x49, 0x55, 0x63, 0x41, 0x00}, // 'W'
    {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41, 0x00}, // 'X'
    {0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00}, // 'Y'
    {0x7F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x7F, 0x00}, // 'Z'
    {0x3C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x00}, // '['
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00}, // '\'
    {0x3C, 0x02, 0x02, 0x02, 0x02, 0x02, 0x3C, 0x00}, // ']'
    {0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00}, // '^'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00}, // '_'
    {0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, // '`'
    {0x00, 0x00, 0x3C, 0x02, 0x3E, 0x42, 0x3E, 0x00}, // 'a' (97)
    {0x40, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x7C, 0x00}, // 'b'
    {0x00, 0x00, 0x3C, 0x40, 0x40, 0x42, 0x3C, 0x00}, // 'c'
    {0x02, 0x02, 0x3E, 0x42, 0x42, 0x42, 0x3E, 0x00}, // 'd'
    {0x00, 0x00, 0x3C, 0x42, 0x7E, 0x40, 0x3C, 0x00}, // 'e'
    {0x1C, 0x22, 0x20, 0x70, 0x20, 0x20, 0x20, 0x00}, // 'f'
    {0x00, 0x00, 0x3E, 0x42, 0x42, 0x3E, 0x02, 0x3C}, // 'g'
    {0x40, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x42, 0x00}, // 'h'
    {0x08, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1C, 0x00}, // 'i'
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x18}, // 'j'
    {0x40, 0x40, 0x44, 0x48, 0x70, 0x48, 0x44, 0x00}, // 'k'
    {0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // 'l'
    {0x00, 0x00, 0x7C, 0x54, 0x54, 0x44, 0x44, 0x00}, // 'm'
    {0x00, 0x00, 0x7C, 0x42, 0x42, 0x42, 0x42, 0x00}, // 'n'
    {0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00}, // 'o'
    {0x00, 0x00, 0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40}, // 'p'
    {0x00, 0x00, 0x3E, 0x42, 0x42, 0x3E, 0x02, 0x02}, // 'q'
    {0x00, 0x00, 0x5C, 0x62, 0x40, 0x40, 0x40, 0x00}, // 'r'
    {0x00, 0x00, 0x3E, 0x40, 0x3C, 0x02, 0x7C, 0x00}, // 's'
    {0x20, 0x20, 0x70, 0x20, 0x20, 0x22, 0x1C, 0x00}, // 't'
    {0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3A, 0x00}, // 'u'
    {0x00, 0x00, 0x42, 0x42, 0x24, 0x24, 0x18, 0x00}, // 'v'
    {0x00, 0x00, 0x42, 0x49, 0x49, 0x36, 0x22, 0x00}, // 'w'
    {0x00, 0x00, 0x42, 0x24, 0x18, 0x24, 0x42, 0x00}, // 'x'
    {0x00, 0x00, 0x42, 0x42, 0x3E, 0x02, 0x3C, 0x00}, // 'y'
    {0x00, 0x00, 0x7E, 0x04, 0x08, 0x10, 0x7E, 0x00}, // 'z'
    {0x0C, 0x10, 0x10, 0x20, 0x10, 0x10, 0x0C, 0x00}, // '{'
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // '|'
    {0x30, 0x08, 0x08, 0x04, 0x08, 0x08, 0x30, 0x00}, // '}'
    {0x38, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // '~'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // DEL
};


// Decode a UTF-8 codepoint. Invalid sequences become '?'.
static uint32_t decodeUtf8Codepoint(const std::string& text, size_t& i) {
    unsigned char c = (unsigned char)text[i++];
    if (c < 0x80) return c;

    auto nextByte = [&]() -> unsigned char {
        if (i >= text.size()) return 0;
        return (unsigned char)text[i++];
    };

    if ((c & 0xE0) == 0xC0) {
        unsigned char c1 = nextByte();
        if ((c1 & 0xC0) != 0x80) return '?';
        return ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(c1 & 0x3F);
    }
    if ((c & 0xF0) == 0xE0) {
        unsigned char c1 = nextByte();
        unsigned char c2 = nextByte();
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return '?';
        return ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(c1 & 0x3F) << 6) | (uint32_t)(c2 & 0x3F);
    }
    if ((c & 0xF8) == 0xF0) {
        unsigned char c1 = nextByte();
        unsigned char c2 = nextByte();
        unsigned char c3 = nextByte();
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return '?';
        return ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(c1 & 0x3F) << 12) | ((uint32_t)(c2 & 0x3F) << 6) | (uint32_t)(c3 & 0x3F);
    }
    return '?';
}

static uint32_t uppercaseCyrillic(uint32_t cp) {
    // а-я -> А-Я
    if (cp >= 0x0430 && cp <= 0x044F) return cp - 0x20;
    // ё -> Ё
    if (cp == 0x0451) return 0x0401;
    return cp;
}

// Minimal 5x7 Cyrillic pixel font. Rows use the low 5 bits.
static const std::array<uint8_t, 7>* cyrillicGlyph(uint32_t cp) {
    cp = uppercaseCyrillic(cp);

    static const std::array<uint8_t, 7> A  = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const std::array<uint8_t, 7> BE = {0x1F,0x10,0x10,0x1E,0x11,0x11,0x1E};
    static const std::array<uint8_t, 7> VE = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const std::array<uint8_t, 7> GE = {0x1F,0x10,0x10,0x10,0x10,0x10,0x10};
    static const std::array<uint8_t, 7> DE = {0x0E,0x12,0x12,0x12,0x12,0x1F,0x11};
    static const std::array<uint8_t, 7> IE = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const std::array<uint8_t, 7> IO = {0x0A,0x00,0x1F,0x10,0x1E,0x10,0x1F};
    static const std::array<uint8_t, 7> ZHE= {0x15,0x15,0x0E,0x04,0x0E,0x15,0x15};
    static const std::array<uint8_t, 7> ZE = {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E};
    static const std::array<uint8_t, 7> I  = {0x11,0x13,0x15,0x15,0x19,0x11,0x11};
    static const std::array<uint8_t, 7> J  = {0x0A,0x04,0x11,0x13,0x15,0x19,0x11};
    static const std::array<uint8_t, 7> KA = {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    static const std::array<uint8_t, 7> EL = {0x07,0x09,0x09,0x09,0x09,0x19,0x11};
    static const std::array<uint8_t, 7> EM = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
    static const std::array<uint8_t, 7> EN = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const std::array<uint8_t, 7> O  = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const std::array<uint8_t, 7> PE = {0x1F,0x11,0x11,0x11,0x11,0x11,0x11};
    static const std::array<uint8_t, 7> ER = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const std::array<uint8_t, 7> ES = {0x0F,0x10,0x10,0x10,0x10,0x10,0x0F};
    static const std::array<uint8_t, 7> TE = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const std::array<uint8_t, 7> U  = {0x11,0x11,0x11,0x0F,0x01,0x01,0x1E};
    static const std::array<uint8_t, 7> EF = {0x04,0x0E,0x15,0x15,0x0E,0x04,0x04};
    static const std::array<uint8_t, 7> HA = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};
    static const std::array<uint8_t, 7> CE = {0x11,0x11,0x11,0x11,0x11,0x1F,0x01};
    static const std::array<uint8_t, 7> CHE= {0x11,0x11,0x11,0x0F,0x01,0x01,0x01};
    static const std::array<uint8_t, 7> SHA= {0x15,0x15,0x15,0x15,0x15,0x15,0x1F};
    static const std::array<uint8_t, 7> SCHA={0x15,0x15,0x15,0x15,0x15,0x1F,0x01};
    static const std::array<uint8_t, 7> HARD={0x18,0x08,0x08,0x0E,0x09,0x09,0x0E};
    static const std::array<uint8_t, 7> Y  = {0x11,0x11,0x19,0x15,0x15,0x15,0x19};
    static const std::array<uint8_t, 7> SOFT={0x10,0x10,0x10,0x1E,0x11,0x11,0x1E};
    static const std::array<uint8_t, 7> E  = {0x0E,0x11,0x01,0x07,0x01,0x11,0x0E};
    static const std::array<uint8_t, 7> YU = {0x12,0x15,0x15,0x1D,0x15,0x15,0x12};
    static const std::array<uint8_t, 7> YA = {0x0F,0x11,0x11,0x0F,0x05,0x09,0x11};

    switch (cp) {
        case 0x0410: return &A;    case 0x0411: return &BE;   case 0x0412: return &VE;
        case 0x0413: return &GE;   case 0x0414: return &DE;   case 0x0415: return &IE;
        case 0x0401: return &IO;   case 0x0416: return &ZHE;  case 0x0417: return &ZE;
        case 0x0418: return &I;    case 0x0419: return &J;    case 0x041A: return &KA;
        case 0x041B: return &EL;   case 0x041C: return &EM;   case 0x041D: return &EN;
        case 0x041E: return &O;    case 0x041F: return &PE;   case 0x0420: return &ER;
        case 0x0421: return &ES;   case 0x0422: return &TE;   case 0x0423: return &U;
        case 0x0424: return &EF;   case 0x0425: return &HA;   case 0x0426: return &CE;
        case 0x0427: return &CHE;  case 0x0428: return &SHA;  case 0x0429: return &SCHA;
        case 0x042A: return &HARD; case 0x042B: return &Y;    case 0x042C: return &SOFT;
        case 0x042D: return &E;    case 0x042E: return &YU;   case 0x042F: return &YA;
        default: return nullptr;
    }
}

void UIRenderer::init(int screenWidth, int screenHeight) {
    width = screenWidth;
    height = screenHeight;

    // Define 2D shaders for UI elements and text
    std::string vertSrc = R"glsl(
        layout(location = 0) in vec2 aPos;
        layout(location = 2) in vec2 aTexCoord;

        out vec2 TexCoord;
        uniform mat4 projection;

        void main() {
            TexCoord = aTexCoord;
            gl_Position = projection * vec4(aPos, 0.0, 1.0);
        }
    )glsl";

    std::string fragSrc = R"glsl(
        in vec2 TexCoord;
        out vec4 FragColor;

        uniform sampler2D uiTexture;
        uniform vec4 uiColor;
        uniform int useTexture;

        void main() {
            if (useTexture != 0) {
                vec4 texel = texture(uiTexture, TexCoord);
                FragColor = vec4(uiColor.rgb, texel.r * uiColor.a);
            } else {
                FragColor = uiColor;
            }
        }
    )glsl";

    uiShader.compile(vertSrc, fragSrc);
    generateFontTexture();

    // Set up standard 2D quad VAO for rendering rectangles
    glGenVertexArrays(1, &rectVao);
    glGenBuffers(1, &rectVbo);

    glBindVertexArray(rectVao);
    glBindBuffer(GL_ARRAY_BUFFER, rectVbo);

    // Coordinate layout: x, y, u, v
    float dummyData[24] = {
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(dummyData), dummyData, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void UIRenderer::resize(int screenWidth, int screenHeight) {
    width = screenWidth;
    height = screenHeight;
}

void UIRenderer::generateFontTexture() {
    // Generate a 128x64 alpha texture from our 8x8 font table
    // 16 columns of characters, 8 rows of characters
    // Grid: 16 * 8 characters. Total ASCII characters = 128 (we support 32 to 127)
    int texW = 128;
    int texH = 64;
    std::vector<uint8_t> pixels(texW * texH, 0);

    for (int charIdx = 0; charIdx < 128; ++charIdx) {
        int fontTableIdx = charIdx - 32;
        if (fontTableIdx < 0 || fontTableIdx >= 96) {
            continue; // Draw empty pixels for non-printable characters
        }

        // Layout character in a 16x8 grid
        int col = charIdx % 16;
        int row = charIdx / 16;

        int startX = col * 8;
        int startY = row * 8;

        for (int y = 0; y < 8; ++y) {
            uint8_t fontRowByte = font8x8_basic[fontTableIdx][y];
            for (int x = 0; x < 8; ++x) {
                // MSB is leftmost pixel
                bool pixelActive = (fontRowByte & (1 << (7 - x))) != 0;
                int pixelX = startX + x;
                int pixelY = startY + y;
                pixels[pixelY * texW + pixelX] = pixelActive ? 255 : 0;
            }
        }
    }

    glGenTextures(1, &fontTextureId);
    glBindTexture(GL_TEXTURE_2D, fontTextureId);

    // Upload as single-channel RED texture, we can use GL_RED on OpenGL ES 3.0 & OpenGL 3.3 Core
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texW, texH, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void UIRenderer::drawRect(float x, float y, float w, float h, const float color[4], GLuint textureId) {
    uiShader.use();

    // Ortho matrix: maps pixel coords (0,0) to (width, height)
    Mat4 ortho;
    ortho.m[0] = 2.0f / width;
    ortho.m[5] = -2.0f / height; // invert Y so (0,0) is top-left
    ortho.m[10] = -1.0f;
    ortho.m[12] = -1.0f;
    ortho.m[13] = 1.0f;
    ortho.m[15] = 1.0f;

    uiShader.setMat4("projection", ortho);

    // Update VBO with exact coordinates
    float data[24] = {
        x,     y + h, 0.0f, 1.0f,
        x + w, y + h, 1.0f, 1.0f,
        x + w, y,     1.0f, 0.0f,

        x,     y + h, 0.0f, 1.0f,
        x + w, y,     1.0f, 0.0f,
        x,     y,     0.0f, 0.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, rectVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);

    GLint locColor = glGetUniformLocation(uiShader.id, "uiColor");
    glUniform4fv(locColor, 1, color);

    if (textureId != 0) {
        uiShader.setInt("useTexture", 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
    } else {
        uiShader.setInt("useTexture", 0);
    }

    // Disable depth testing for UI overlay rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(rectVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void UIRenderer::drawText(const std::string& text, float x, float y, float scale, const float color[4]) {
    uiShader.use();

    Mat4 ortho;
    ortho.m[0] = 2.0f / width;
    ortho.m[5] = -2.0f / height; // invert Y so (0,0) is top-left
    ortho.m[10] = -1.0f;
    ortho.m[12] = -1.0f;
    ortho.m[13] = 1.0f;
    ortho.m[15] = 1.0f;
    uiShader.setMat4("projection", ortho);

    GLint locColor = glGetUniformLocation(uiShader.id, "uiColor");
    glUniform4fv(locColor, 1, color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTextureId);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(rectVao);

    float currentX = x;
    float charW = 8.0f * scale;
    float charH = 8.0f * scale;
    uiShader.setInt("useTexture", 1);
    bool textureMode = true;

    auto setTextureMode = [&](bool enabled) {
        if (textureMode != enabled) {
            uiShader.setInt("useTexture", enabled ? 1 : 0);
            textureMode = enabled;
            if (enabled) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fontTextureId);
            }
        }
    };

    auto drawQuad = [&](float qx, float qy, float qw, float qh, float u0, float v0, float u1, float v1) {
        float data[24] = {
            qx,      qy + qh, u0, v1,
            qx + qw, qy + qh, u1, v1,
            qx + qw, qy,      u1, v0,

            qx,      qy + qh, u0, v1,
            qx + qw, qy,      u1, v0,
            qx,      qy,      u0, v0
        };
        glBindBuffer(GL_ARRAY_BUFFER, rectVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };

    auto drawAscii = [&](uint32_t cp) {
        if (cp < 32 || cp > 127) cp = '?';
        setTextureMode(true);

        int col = (int)(cp % 16);
        int row = (int)(cp / 16);
        float u_start = col / 16.0f;
        float u_end = (col + 1) / 16.0f;
        float v_start = row / 8.0f;
        float v_end = (row + 1) / 8.0f;

        drawQuad(currentX, y, charW, charH, u_start, v_start, u_end, v_end);
    };

    auto drawCyrillic = [&](const std::array<uint8_t, 7>& glyph) {
        setTextureMode(false);
        const float pixel = scale;
        const float offsetX = currentX + 1.5f * scale;
        const float offsetY = y + 0.5f * scale;

        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[row] & (1 << (4 - col))) {
                    drawQuad(offsetX + col * pixel, offsetY + row * pixel,
                             pixel, pixel, 0.0f, 0.0f, 1.0f, 1.0f);
                }
            }
        }
    };

    for (size_t i = 0; i < text.size();) {
        uint32_t cp = decodeUtf8Codepoint(text, i);
        if (cp == '\n') {
            y += charH * 1.5f;
            currentX = x;
            continue;
        }

        // Common Cyrillic punctuation in UTF-8 text.
        if (cp == 0x2014 || cp == 0x2013) cp = '-';
        if (cp == 0x2026) cp = '.';
        if (cp == 0x00AB || cp == 0x00BB || cp == 0x201C || cp == 0x201D) cp = '"';
        if (cp == 0x2018 || cp == 0x2019) cp = '\'';
        if (cp == 0x00A0) cp = ' ';

        if (cp < 128) {
            drawAscii(cp);
        } else if (const auto* glyph = cyrillicGlyph(cp)) {
            drawCyrillic(*glyph);
        } else {
            drawAscii('?');
        }

        currentX += charW;
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void UIRenderer::drawVirtualJoysticks(float leftX, float leftY, float leftRadius, float activeX, float activeY,
                                      float rightX, float rightY, float rightRadius, float activeCamX, float activeCamY,
                                      bool isAndroid, const std::string& actionLabel) {
    if (!isAndroid) return;

    // Draw Left joystick (movement)
    float baseColor[4] = {0.3f, 0.3f, 0.3f, 0.4f};
    float innerColor[4] = {0.5f, 0.5f, 0.5f, 0.6f};

    // Draw background circle (using rects as visual proxy, but let's make it look like a nice cross or circles)
    drawRect(leftX - leftRadius, leftY - leftRadius, leftRadius * 2.0f, leftRadius * 2.0f, baseColor);
    drawRect(activeX - 20.0f, activeY - 20.0f, 40.0f, 40.0f, innerColor);

    // Draw Right joystick / Touchpad region (look area)
    float rightBaseColor[4] = {0.2f, 0.2f, 0.2f, 0.2f};
    drawRect(rightX - rightRadius, rightY - rightRadius, rightRadius * 2.0f, rightRadius * 2.0f, rightBaseColor);
    drawRect(activeCamX - 25.0f, activeCamY - 25.0f, 50.0f, 50.0f, innerColor);

    // On-screen Action/Interact Button (for Android sleeping/picking logs)
    float actionColor[4] = {0.8f, 0.2f, 0.2f, 0.5f};
    // Let's place the Interact button on the lower-right side, above the look trackpad
    drawRect(width - 160.0f, height - 320.0f, 100.0f, 100.0f, actionColor);
    
    float actionTextColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    drawText(actionLabel, width - 150.0f, height - 280.0f, 1.5f, actionTextColor);
}

void UIRenderer::cleanup() {
    uiShader.cleanup();
    if (fontTextureId != 0) {
        glDeleteTextures(1, &fontTextureId);
        fontTextureId = 0;
    }
    if (rectVao != 0) {
        glDeleteVertexArrays(1, &rectVao);
        glDeleteBuffers(1, &rectVbo);
        rectVao = rectVbo = 0;
    }
}
