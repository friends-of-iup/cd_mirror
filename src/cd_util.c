/** \file
 * \brief Utilities
 *
 * See Copyright Notice in cd.h
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>


#include "cd.h"

#include "cd_private.h"


int cdRound(double x)
{
  return _cdRound(x);
}

/* Returns a table to speed up zoom in discrete zoom rotines.
   Adds the min parameter and allocates the table using malloc.
   The table should be used when mapping from destiny coordinates to
   source coordinates (src_pos = tab[dst_pos]).
   dst_len is the full destiny size range.
   src_len is only the zoomed region size, usually max-min+1.
*/
int* cdGetZoomTable(int dst_len, int src_len, int src_min)
{
  int dst_i, src_i;
  double factor;
	int* tab = (int*)malloc(dst_len*sizeof(int));

  factor = (double)(src_len) / (double)(dst_len);

	for(dst_i = 0; dst_i < dst_len; dst_i++)
  {
    src_i = cdRound((factor*(dst_i + 0.5)) - 0.5);
		tab[dst_i] = src_i + src_min;
  }

  return tab;
}

/* funcao usada para calcular os retangulos efetivos de zoom 
   de imagens clientes. Pode ser usada para os eixos X e Y.

   canvas_size - tamanho do canvas (canvas->w, canvas->h)
   cnv_rect_pos - posicao no canvas onde a regiao sera� desenhada (x, y)
   cnv_rect_size - tamanho da regiao no canvas com zoom (w, h)
   img_rect_pos - posicao na imagem da regiao a ser desenhada (min)
   img_rect_size - tamanho da regiao na imagem (max-min+1)

   calcula o melhor tamanho a ser usado
   retorna 0 se o retangulo resultante e� nulo
*/
int cdCalcZoom(int canvas_size,
               int cnv_rect_pos, int cnv_rect_size, 
               int *new_cnv_rect_pos, int *new_cnv_rect_size, 
               int img_rect_pos, int img_rect_size, 
               int *new_img_rect_pos, int *new_img_rect_size, 
               int is_horizontal)
{
  int offset;
  double zoom_factor = (double)img_rect_size / (double)cnv_rect_size;

  /* valores default sem otimizacao */
  *new_cnv_rect_size = cnv_rect_size, *new_cnv_rect_pos = cnv_rect_pos;    
  *new_img_rect_size = img_rect_size, *new_img_rect_pos = img_rect_pos;

  if (cnv_rect_size > 0)
  {
    /* se posicao no canvas > tamanho do canvas, fora da janela, nao desenha nada */
    if (cnv_rect_pos >= canvas_size) 
      return 0;

    /* se posicao no canvas + tamanho da regiao no canvas < 0, fora da janela, nao desenha nada */
    if (cnv_rect_pos+cnv_rect_size < 0) 
      return 0;

    /* se posicao no canvas < 0, entao comeca fora do canvas melhor posicao no canvas e' 0
                                 E o tamanho e' reduzido do valor negativo */
    if (cnv_rect_pos < 0) 
    {
      /* valores ajustados para cair numa vizinhanca de um pixel da imagem */
      offset = (int)ceil(cnv_rect_pos*zoom_factor);   /* offset is <0 */
      offset = (int)ceil(offset/zoom_factor);
      *new_cnv_rect_pos -= offset;  
      *new_cnv_rect_size += offset; 
    }

    /* se posicao no canvas + tamanho da regiao no canvas > tamanho do canvas, 
       termina fora do canvas entao 
       o tamanho da regiao no canvas e' o tamanho do canvas reduzido da posicao */
    if (*new_cnv_rect_pos+*new_cnv_rect_size > canvas_size) 
    {
      offset = (int)((*new_cnv_rect_pos+*new_cnv_rect_size - canvas_size)*zoom_factor);
      *new_cnv_rect_size -= (int)(offset/zoom_factor);  /* offset is >0 */
    }
  }
  else
  {
    /* cnv_rect_size tamanho negativo, significa imagem top down */
    /* calculos adicionados pela Paula */

    /* se posicao no canvas + tamanho no canvas (xmin+1) >= tamanho do canvas, fora da janela, nao desenha nada */
    if (cnv_rect_pos+cnv_rect_size >= canvas_size) 
      return 0;

    /* se posicao da imagem com zoom (xmax) < 0, fora da janela, nao desenha nada */
    if (cnv_rect_pos < 0) 
      return 0;

    /* se posicao com zoom (xmax) >= tamanho do canvas, posicao da imagem com zoom e' o tamanho do canvas menos um
       tambem o tamanho e' reduzido do valor negativo */
    if (cnv_rect_pos >= canvas_size) 
    {
      *new_cnv_rect_pos = canvas_size-1; 
      *new_cnv_rect_size = cnv_rect_size + (cnv_rect_pos - *new_cnv_rect_pos);
    }

    /* se posicao + tamanho com zoom (xmin+1) < 0, 
       entao o tamanho com zoom e' a posi��o + 1 */
    if (cnv_rect_pos+cnv_rect_size < 0) 
      *new_cnv_rect_size = -(*new_cnv_rect_pos + 1);
  }

  /* agora ja' tenho tamanho e posicao da regiao no canvas,
     tenho que obter tamanho e posicao dentro da imagem original,
     baseado nos novos valores */

  /* tamanho da regiao na imagem e' a conversao de zoom para real do tamanho no canvas */
  *new_img_rect_size = (int)(*new_cnv_rect_size * zoom_factor + 0.5);

  if (is_horizontal)
  {
    /* em X, o offset dentro da imagem so' existe se houver diferenca entre a posicao inicial da
       imagem e a posicao otimizada (ambas da imagem com zoom) */
    if (*new_cnv_rect_pos != cnv_rect_pos)
    {
      offset = *new_cnv_rect_pos - cnv_rect_pos; /* offset is >0 */
      *new_img_rect_pos += (int)(offset*zoom_factor);
    }
  }
  else
  {
    /* em Y, o offset dentro da imagem so' existe se houver diferenca entre a posicao 
       final (posi��o inicial + tamanho) da imagem e a posicao otimizada (ambas da 
       imagem com zoom) */
    if ((cnv_rect_pos + cnv_rect_size) != (*new_cnv_rect_pos + *new_cnv_rect_size))
    {
      /* offset is >0, because Y axis is from top to bottom */
      offset = (cnv_rect_pos + cnv_rect_size) - (*new_cnv_rect_pos + *new_cnv_rect_size);
      *new_img_rect_pos += (int)(offset*zoom_factor);
    }
  }

  return 1;
}

int cdGetFileName(const char* strdata, char* filename)
{
  const char* start = strdata;
  if (!strdata || strdata[0] == 0) return 0;
  
  if (strdata[0] == '\"')
  {   
    strdata++; /* the first " */
    while(*strdata && *strdata != '\"')
      *filename++ = *strdata++;
    strdata++; /* the last " */
  }
  else
  {
    while(*strdata && *strdata != ' ')
      *filename++ = *strdata++;
  }

  if (*strdata == ' ')
    strdata++;

  *filename = 0;
  return (int)(strdata - start);
}

void cdNormalizeLimits(int w, int h, int *xmin, int *xmax, int *ymin, int *ymax)
{
  *xmin = *xmin < 0? 0: *xmin < w? *xmin: (w - 1);
  *ymin = *ymin < 0? 0: *ymin < h? *ymin: (h - 1);
  *xmax = *xmax < 0? 0: *xmax < w? *xmax: (w - 1);
  *ymax = *ymax < 0? 0: *ymax < h? *ymax: (h - 1);
}

int cdCheckBoxSize(int *xmin, int *xmax, int *ymin, int *ymax)
{
  if (*xmin > *xmax) _cdSwapInt(*xmin, *xmax);
  if (*ymin > *ymax) _cdSwapInt(*ymin, *ymax);

  if ((*xmax-*xmin+1) <= 0)
    return 0;

  if ((*ymax-*ymin+1) <= 0)
    return 0;

  return 1;
}

int cdfCheckBoxSize(double *xmin, double *xmax, double *ymin, double *ymax)
{
  if (*xmin > *xmax) _cdSwapDouble(*xmin, *xmax);
  if (*ymin > *ymax) _cdSwapDouble(*ymin, *ymax);

  if ((*xmax-*xmin+1) <= 0)
    return 0;

  if ((*ymax-*ymin+1) <= 0)
    return 0;

  return 1;
}

void cdMovePoint(int *x, int *y, double dx, double dy, double sin_theta, double cos_theta)
{
  double t;
  t = cos_theta*dx - sin_theta*dy;
  *x += _cdRound(t);
  t = sin_theta*dx + cos_theta*dy;
  *y += _cdRound(t);
}

void cdfMovePoint(double *x, double *y, double dx, double dy, double sin_theta, double cos_theta)
{
  *x += cos_theta*dx - sin_theta*dy;
  *y += sin_theta*dx + cos_theta*dy;
}

void cdRotatePoint(cdCanvas* canvas, int x, int y, int cx, int cy, int *rx, int *ry, double sin_theta, double cos_theta)
{
  double t;

  /* translate to (cx,cy) */
  x = x - cx;
  y = y - cy;

  /* rotate */
  if (canvas->invert_yaxis)
  {
    t =  (x * cos_theta) + (y * sin_theta); *rx = _cdRound(t); 
    t = -(x * sin_theta) + (y * cos_theta); *ry = _cdRound(t);
  }
  else
  {
    t = (x * cos_theta) - (y * sin_theta); *rx = _cdRound(t); 
    t = (x * sin_theta) + (y * cos_theta); *ry = _cdRound(t); 
  }

  /* translate back */
  *rx = *rx + cx;
  *ry = *ry + cy;
}

void cdfRotatePoint(cdCanvas* canvas, double x, double y, double cx, double cy, double *rx, double *ry, double sin_theta, double cos_theta)
{
  /* translate to (cx,cy) */
  x = x - cx;
  y = y - cy;

  /* rotate */
  if (canvas->invert_yaxis)
  {
    *rx =  (x * cos_theta) + (y * sin_theta); 
    *ry = -(x * sin_theta) + (y * cos_theta); 
  }
  else
  {
    *rx = (x * cos_theta) - (y * sin_theta); 
    *ry = (x * sin_theta) + (y * cos_theta); 
  }

  /* translate back */
  *rx = *rx + cx;
  *ry = *ry + cy;
}

void cdRotatePointY(cdCanvas* canvas, int x, int y, int cx, int cy, int *ry, double sin_theta, double cos_theta)
{
  double t;

  /* translate to (cx,cy) */
  x = x - cx;
  y = y - cy;

  /* rotate */
  if (canvas->invert_yaxis)
  {
    t = -(x * sin_theta) + (y * cos_theta); *ry = _cdRound(t);
  }
  else
  {
    t = (x * sin_theta) + (y * cos_theta); *ry = _cdRound(t); 
  }

  /* translate back */
  *ry = *ry + cy;
}

/* Copied from IUP3 */

int cdStrEqualNoCase(const char* str1, const char* str2) 
{
  int i = 0;
  if (str1 == str2) return 1;
  if (!str1 || !str2 || tolower(*str1) != tolower(*str2)) return 0;

  while (str1[i] && str2[i] && tolower(str1[i])==tolower(str2[i])) 
    i++;
  if (str1[i] == str2[i]) return 1; 

  return 0;
}

int cdStrEqualNoCasePartial(const char* str1, const char* str2) 
{
  int i = 0;
  if (str1 == str2) return 1;
  if (!str1 || !str2 || tolower(*str1) != tolower(*str2)) return 0;

  while (str1[i] && str2[i] && tolower(str1[i])==tolower(str2[i])) 
    i++;
  if (str1[i] == str2[i]) return 1; 
  if (str2[i] == 0) return 1;

  return 0;
}

/* Copied from IUP3, simply ignore line breaks other than '\n' for CD */

int cdStrLineCount(const char* str)
{
  int num_lin = 1;

  if (!str)
    return num_lin;

  while(*str != 0)
  {
    while(*str!=0 && *str!='\n')
      str++;

    if (*str=='\n')   /* UNIX line end */
    {
      num_lin++;
      str++;
    }
  }

  return num_lin;
}

char* cdStrDup(const char *str)
{
  if (str)
  {
    int size = strlen(str)+1;
    char *newstr = malloc(size);
    if (newstr) memcpy(newstr, str, size);
    return newstr;
  }
  return NULL;
}

char* cdStrDupN(const char *str, int len)
{
  if (str)
  {
    int size = len+1;
    char *newstr = malloc(size);
    if (newstr) 
    {
      memcpy(newstr, str, len);
      newstr[len]=0;
    }
    return newstr;
  }
  return NULL;
}

int cdStrIsAscii(const char* str)
{
  while(*str)
  {
    int c = *str;
    if (c < 0)
      return 0;
    str++;
  }
  return 1;
}

void cdSetPaperSize(int size, double *w_pt, double *h_pt)
{
  static struct
  {
    int w_pt;
    int h_pt;
  } paper[] =
    {
      { 2393, 3391 },   /*   A0   */
      { 1689, 2393 },   /*   A1   */
      { 1192, 1689 },   /*   A2   */
      {  842, 1192 },   /*   A3   */
      {  595,  842 },   /*   A4   */
      {  420,  595 },   /*   A5   */
      {  612,  792 },   /* LETTER */
      {  612, 1008 }    /*  LEGAL */
    };

  if (size<CD_A0 || size>CD_LEGAL) 
    return;

  *w_pt = (double)paper[size].w_pt;
  *h_pt = (double)paper[size].h_pt;
}

#ifdef WIN32
#include <windows.h>
static char* winRegReadStringKey(HKEY hBaseKey, const char* key_name, const char* value_name)
{
	HKEY hKey;
	DWORD size;
  char* str;

	if (RegOpenKeyExA(hBaseKey, key_name, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return NULL;

  if (RegQueryValueExA(hKey, value_name, NULL, NULL, NULL, &size) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
		return NULL;
  }

  str = malloc(size);
  RegQueryValueExA(hKey, value_name, NULL, NULL, (LPBYTE)str, &size);

	RegCloseKey(hKey);
	return str;
}

static int winRegGetValueCount(HKEY hKey, int *count, int *max_name_size, int *max_value_size)
{
  DWORD cValues;
  DWORD cMaxValueNameLen;
  DWORD cMaxValueLen;

  if (RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
      &cValues,             // number of values for this hKey 
      &cMaxValueNameLen,    // longest value name 
      &cMaxValueLen,        // longest value data 
      NULL, NULL) == ERROR_SUCCESS)
  {
    if (!cValues)
      return 0;

    *count = (int)cValues;
    *max_name_size = (int)cMaxValueNameLen;
    *max_value_size = (int)cMaxValueLen;
    return 1;
  }
  else
    return 0;
}

typedef char regName[MAX_PATH];

static int iCompareStr(const void *a, const void *b)
{
  const regName*aa = a;
  const regName*bb = b;
  return strcmp(*aa, *bb);
}

static char* winRegFindValue(HKEY hBaseKey, const char* key_name, const char* value_name)
{
	HKEY hKey;
  int i, count, max_name_size, max_value_size;
  DWORD cchValueName;
  regName* ValueNames;

	if (RegOpenKeyExA(hBaseKey, key_name, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return NULL;

  if (!winRegGetValueCount(hKey, &count, &max_name_size, &max_value_size))
    return NULL;

  ValueNames = malloc(sizeof(regName)*count);

  for (i=0; i<count; i++)
  {
    cchValueName = MAX_PATH-1;

    if (RegEnumValueA(hKey, i, 
        ValueNames[i], &cchValueName, 
        NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
      break;
  }
  count = i;

  /* must sort to be able to do a partial compare */
  qsort(ValueNames, count, sizeof(regName), iCompareStr);

  RegCloseKey(hKey);

  for (i = 0; i<count; i++)
  {
    if (cdStrEqualNoCasePartial(ValueNames[i], value_name))
    {
      char* lpData = winRegReadStringKey(hBaseKey, key_name, ValueNames[i]);
      free(ValueNames);
      return lpData;
    }
  }

  free(ValueNames);
	return NULL;
}

int cdGetFontFileNameSystem(const char *type_face, int style, char* filename)
{
  char win_font_name[1024];
  char *font_dir, *font_title;

  font_dir = winRegReadStringKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Fonts");
  if (!font_dir)
    return 0;

  /* Filter some pre-defined names */
  if (cdStrEqualNoCase(type_face, "Courier") || 
      cdStrEqualNoCase(type_face, "Monospace") ||
      cdStrEqualNoCase(type_face, "System"))
    type_face = "Courier New";
  else if (cdStrEqualNoCase(type_face, "Times") || 
           cdStrEqualNoCase(type_face, "Serif"))
    type_face = "Times New Roman";
  else if (cdStrEqualNoCase(type_face, "Helvetica") || 
           cdStrEqualNoCase(type_face, "Sans"))
    type_face = "Arial";

  strcpy(win_font_name, type_face);

  /* add style */
  if (style & CD_BOLD)
    strcat(win_font_name, " Bold");
  if (style & CD_ITALIC)
    strcat(win_font_name, " Italic");

  font_title = winRegFindValue(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", win_font_name);
  if (font_title)
  {
    /* font_title already includes file extension, but may also contains a path */
    if (strchr(font_title, ':'))
      strcpy(filename, font_title);
    else
      sprintf(filename, "%s\\%s", font_dir, font_title);  
    free(font_title);
    free(font_dir);
    return 1;
  }

  free(font_dir);
  return 0;
}
#else
#ifndef NO_FONTCONFIG
#include <fontconfig/fontconfig.h>

int cdGetFontFileNameSystem(const char *type_face, int style, char* filename)
{
  char styles[4][20];
  int style_size;
  FcObjectSet *os;
  FcFontSet *fs;
  FcPattern *pat;
  int j, s, found = 0;

  /* Filter some pre-defined names */
  if (cdStrEqualNoCase(type_face, "Courier") || 
      cdStrEqualNoCase(type_face, "Courier New") || 
      cdStrEqualNoCase(type_face, "Monospace") ||
      cdStrEqualNoCase(type_face, "System"))
    type_face = "freemono";
  else if (cdStrEqualNoCase(type_face, "Times") || 
           cdStrEqualNoCase(type_face, "Times New Roman")|| 
           cdStrEqualNoCase(type_face, "Serif"))
    type_face = "freeserif";
  else if (cdStrEqualNoCase(type_face, "Helvetica") || 
           cdStrEqualNoCase(type_face, "Arial") || 
           cdStrEqualNoCase(type_face, "Sans"))
    type_face = "freesans";

  /* add style */
  if( style&CD_BOLD && style&CD_ITALIC )
  {
    strcpy(styles[0], "BoldItalic");
    strcpy(styles[1], "Bold Italic");
    strcpy(styles[2], "Bold Oblique");
    strcpy(styles[3], "BoldOblique");
    style_size = 4;
  }
  else if( style & CD_BOLD )
  {
    strcpy(styles[0], "Bold");
    style_size = 1;
  }
  else if( style & CD_ITALIC )
  {
    strcpy(styles[0], "Italic");
    strcpy(styles[1], "Oblique");
    style_size = 2;
  }
  else
  {
    strcpy(styles[0], "Regular");
    strcpy(styles[1], "Normal");
    strcpy(styles[2], "Medium");
    style_size = 3;
  }

  pat = FcPatternCreate();
  os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_STYLE, NULL);
  fs = FcFontList(NULL, pat, os);
  if (pat) FcPatternDestroy(pat);
  if (os) FcObjectSetDestroy(os);

  if(!fs)
    return 0;

  /* for all installed fonts */
  for (j = 0; j < fs->nfont; j++)
  {
    FcChar8 *family;
    FcPatternGetString(fs->fonts[j], FC_FAMILY, 0, &family );

    if (cdStrEqualNoCasePartial((char*)family, type_face))
    {
      FcChar8 *file;
      FcChar8 *style;
      FcPatternGetString(fs->fonts[j], FC_FILE, 0, &file); 
      FcPatternGetString(fs->fonts[j], FC_STYLE, 0, &style );

      /* for all styles of that family */
      for(s = 0; s < style_size; s++ )
      {
        if (cdStrEqualNoCase(styles[s], (char*)style))
        {
          strcpy(filename, (char*)file);
          FcFontSetDestroy (fs);
          return 1;
        }
      }

      strcpy(filename, (char*)file);
      found = 1;  /* ignore styles */
    }
  }

  FcFontSetDestroy (fs);
  return found;
}
#else
int cdGetFontFileNameSystem(const char *type_face, int style, char* filename)
{
  (void)type_face;
  (void)style;
  (void)filename;
  return 0;
}
#endif
#endif

int cdGetFontFileNameDefault(const char *type_face, int style, char* filename)
{
  char font[10240];
  static char * cd_ttf_font_style[4] = {
    "",
    "bd",
    "i",
    "bi"};
  const char* face;

  /* check for the pre-defined names */
  if (cdStrEqualNoCase(type_face, "Courier") ||
      cdStrEqualNoCase(type_face, "System"))
    face = "cour";
  else if (cdStrEqualNoCase(type_face, "Times"))
    face = "times";
  else if (cdStrEqualNoCase(type_face, "Helvetica"))
    face = "arial";
  else
    face = type_face;

  sprintf(font, "%s%s", face, cd_ttf_font_style[style&3]);
  if (!cdGetFontFileName(font, filename))
  {
    /* try the type_face as a file title, but with no style */
    if (face == type_face && style != CD_PLAIN)
      return cdGetFontFileName(type_face, filename);
    else
      return 0;
  }
  else 
    return 1;
}

int cdGetFontFileName(const char* type_face, char* filename)
{
  FILE *file;

  if (!type_face)
    return 0;

  /* current directory */
  sprintf(filename, "%s.ttf", type_face);
  file = fopen(filename, "r");

  if (file)
    fclose(file);
  else
  {
    /* CD environment */
    char* env = getenv("CDDIR");
    if (env)
    {
#ifdef WIN32
      sprintf(filename, "%s\\%s.ttf", env, type_face);
#else
      sprintf(filename, "%s/%s.ttf", env, type_face);
#endif
      file = fopen(filename, "r");
    }

    if (file)
      fclose(file);
    else
    {
#ifdef WIN32
      /* Windows Font folder */
      char* font_dir = winRegReadStringKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Fonts");
      sprintf(filename, "%s\\%s.ttf", font_dir, type_face);
      file = fopen(filename, "r");
      free(font_dir);

      if (file)
        fclose(file);
      else
        return 0;
#else
      return 0;
#endif
    }
  }

  return 1;
}

int cdStrTmpFileName(char* filename)
{
#ifdef WIN32
  char tmpPath[10240];
  if (GetTempPathA(10240, tmpPath)==0)
    return 0;
  if (GetTempFileNameA(tmpPath, "~cd", 0, filename)==0)
    return 0;
  return 1;
#else
  return tmpnam(filename)!=NULL;
#endif
}
