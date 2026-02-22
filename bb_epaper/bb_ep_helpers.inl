#ifndef __BB_EP_HELPERS__
#define __BB_EP_HELPERS__

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void bbepSetFont(BBEPDISP *pBBEP, int iFont)
{
    pBBEP->iFont = iFont;
    pBBEP->pFont = NULL;
}

void bbepSetFontP(BBEPDISP *pBBEP, const void *pFont)
{
    pBBEP->iFont = -1;
    pBBEP->pFont = (void *)pFont;
}

void bbepSetTextColor(BBEPDISP *pBBEP, int iFG, int iBG)
{
    pBBEP->iFG = iFG;
    pBBEP->iBG = (iBG == -1) ? iFG : iBG;
}

size_t bbepWrite(BBEPDISP *pBBEP, uint8_t c)
{
    char szTemp[2]; // used to draw 1 character at a time to the C methods
    int w = 8, h = 8;
    static int iUnicodeCount = 0;
    static uint8_t u8Unicode0, u8Unicode1;

    if (iUnicodeCount == 0)
    {
        if (c >= 0x80)
        { // start of a multi-byte character
            iUnicodeCount++;
            u8Unicode0 = c;
            return 1;
        }
    }
    else
    { // middle/end of a multi-byte character
        uint16_t u16Code;
        if (u8Unicode0 < 0xe0)
        { // 2 byte char, 0-0x7ff
            u16Code = (u8Unicode0 & 0x3f) << 6;
            u16Code += (c & 0x3f);
            c = bbepUnicodeTo1252(u16Code);
            iUnicodeCount = 0;
        }
        else
        { // 3 byte character 0x800 and above
            if (iUnicodeCount == 1)
            {
                iUnicodeCount++; // save for next byte to arrive
                u8Unicode1 = c;
                return 1;
            }
            u16Code = (u8Unicode0 & 0x3f) << 12;
            u16Code += (u8Unicode1 & 0x3f) << 6;
            u16Code += (c & 0x3f);
            c = bbepUnicodeTo1252(u16Code);
            iUnicodeCount = 0;
        }
    }
    szTemp[0] = c;
    szTemp[1] = 0;
    if (pBBEP->pFont == NULL)
    { // use built-in fonts
        if (pBBEP->iFont == FONT_8x8 || pBBEP->iFont == FONT_6x8)
        {
            h = 8;
            w = (pBBEP->iFont == FONT_8x8) ? 8 : 6;
        }
        else if (pBBEP->iFont == FONT_12x16 || pBBEP->iFont == FONT_16x16)
        {
            h = 16;
            w = (pBBEP->iFont == FONT_12x16) ? 12 : 16;
        }

        if (c == '\n')
        {                         // Newline?
            pBBEP->iCursorX = 0;  // Reset x to zero,
            pBBEP->iCursorY += h; // advance y one line
        }
        else if (c != '\r')
        { // Ignore carriage returns
            if (pBBEP->wrap && ((pBBEP->iCursorX + w) > pBBEP->width))
            {                         // Off right?
                pBBEP->iCursorX = 0;  // Reset x to zero,
                pBBEP->iCursorY += h; // advance y one line
            }
            bbepWriteString(pBBEP, -1, -1, szTemp, pBBEP->iFont, pBBEP->iFG, pBBEP->iBG);
        }
    }
    else
    { // Custom font
        BB_FONT *pBBF;
        BB_FONT_SMALL *pBBFS;
        BB_GLYPH *pGlyph;
        BB_GLYPH_SMALL *pSmallGlyph;
        int first, last;
        if (pgm_read_word(pBBEP->pFont) == BB_FONT_MARKER)
        {
            pBBF = (BB_FONT *)pBBEP->pFont;
            pBBFS = NULL;
            first = pgm_read_byte(&pBBF->first);
            last = pgm_read_byte(&pBBF->last);
        }
        else
        { // small font
            pBBFS = (BB_FONT_SMALL *)pBBEP->pFont;
            pBBF = NULL;
            first = pgm_read_byte(&pBBFS->first);
            last = pgm_read_byte(&pBBFS->last);
        }
        if (c == '\n')
        {
            pBBEP->iCursorX = 0;
            if (pBBF)
            {
                pBBEP->iCursorY += pBBF->height;
            }
            else
            {
                pBBEP->iCursorY += pBBFS->height;
            }
        }
        else if (c != '\r')
        {
            if (c >= first && c <= last)
            {
                if (pBBF)
                {
                    pGlyph = &pBBF->glyphs[c - first];
                    w = pgm_read_word(&pGlyph->width);
                    h = pgm_read_word(&pGlyph->height);
                }
                else
                { // small font
                    pSmallGlyph = &pBBFS->glyphs[c - first];
                    w = pgm_read_word(&pSmallGlyph->width);
                    h = pgm_read_word(&pSmallGlyph->height);
                }
                if (w > 0 && h > 0)
                { // Is there an associated bitmap?
                    if (pBBF)
                    {
                        w += (int16_t)pgm_read_word(&pGlyph->xOffset);
                    }
                    else
                    {
                        w += (int8_t)pgm_read_byte(&pSmallGlyph->xOffset);
                    }
                    if (pBBEP->wrap && (pBBEP->iCursorX + w) > pBBEP->width)
                    {
                        pBBEP->iCursorX = 0;
                        pBBEP->iCursorY += h;
                    }
                    bbepWriteStringCustom(pBBEP, pBBEP->pFont, -1, -1, szTemp, pBBEP->iFG, pBBEP->iPlane);
                }
            }
        }
    }
    return 1;
}

size_t bbepPrint(BBEPDISP *pBBEP, const char *pString)
{
    uint8_t *s = (uint8_t *)pString;
    size_t count = 0;

    while (*s != 0)
    {
        count += bbepWrite(pBBEP, *s++);
    }
    return count;
}

size_t bbepPrintln(BBEPDISP *pBBEP, const char *pString)
{
    char ucTemp[4];
    size_t count = 0;

    count += bbepPrint(pBBEP, pString);
    ucTemp[0] = '\n';
    ucTemp[1] = '\r';
    ucTemp[2] = 0;
    count += bbepPrint(pBBEP, (const char *)ucTemp);
    return count;
}

size_t bbepPrintf(BBEPDISP *pBBEP, const char *fmt, ...)
{
    char printbuffer[256]; // resulting string limited to 256 chars
    va_list va;
    va_start(va, fmt);
    vsnprintf(printbuffer, sizeof(printbuffer), fmt, va);
    va_end(va);
    printbuffer[sizeof(printbuffer) - 1] = 0; // ensure null termination
    return bbepPrint(pBBEP, printbuffer);
}

#endif