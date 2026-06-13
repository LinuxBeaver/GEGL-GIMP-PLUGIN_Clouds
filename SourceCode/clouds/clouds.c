/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Credit to Øyvind Kolås (pippin) for major GEGL contributions
 * 2023 Beaver  GEGL Clouds
 */

/*
graph 1 (original)


noise-solid detail=10 turbulent=22  seed=54 tileable=true
hue-chroma lightness=-22
value-invert
id=1
hard-light aux=[ ref=1 color  value=#c3f2ff  ]
opacity value=4
id=2
hard-light aux=[ ref=2 color  high-pass ]
saturation scale=0.6


(graph 2 realistic)


 id=1 
src aux=[ ref=1
noise-solid x-size=33 y-size=33 seed=33
emboss depth=3
gimp:layer-mode layer-mode=hsl-color composite-mode=auto aux=[ color value=#00d0ff  ]
gimp:layer-mode layer-mode=addition opacity=0.1 composite-mode=auto aux=[ noise-solid x-size=3 y-size=3  ]
]
saturation scale=0.5
crop aux=[ ref=1 ]
gimp:layer-mode layer-mode=overlay composite-mode=auto blend-space=rgb-linear aux=[ noise-solid x-size=2 y-size=2 detail=4  invert-gamma  multiply value=1.5
opacity value=2.1 ]
gaussian-blur std-dev-x=1 std-dev-y=1

graph 3 (night time)


id=0 noise-solid detail=15
crop aux=[ ref=0 ]
id=1 gimp:layer-mode layer-mode=luminance composite-mode=auto aux=[ ref=1  hue-chroma lightness=-13 ]
divide value=2.9
multiply value=3
gimp:layer-mode layer-mode=addition composite-mode=auto aux=[ color value=#0a0065 ] 
gamma value=1.8
saturation scale=0.7
id=y
screen aux=[  ref=y
id=x src  aux=[ ref=x  ] 
crop aux=[ ref=y  ]
color-overlay value=white
noise-hsv holdness=8 value-distance=0.055 saturation-distance=0
invert
levels in-high=0.33 out-high=4.15
gamma value=20
gaussian-blur std-dev-x=1 std-dev-y=1
opacity value=0.4 ]
 
*/


#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


property_enum (cloud_type, _("Cloud type"),
    cloud_list, cloudlist,
    original)
   description  (_("Select the cloud graph to use"))


enum_start (cloudlist)
  enum_value (original,      "original",
              N_("Original"))
  enum_value (realistic,      "realistic",
              N_("Realistic"))
  enum_value (night,      "night",
              N_("Night time"))
enum_end (cloud_list)

property_double (cloudsize, _("Scale clouds"), 4.0)
    description (_("Size of the clouds"))
    value_range (0.5, 14.0)
    ui_range    (1.5, 9.0)
    ui_meta     ("unit", "pixel-distance")
  ui_steps      (0.01, 0.50)
    ui_meta ("visible", "cloud_type {original}")

property_double (cloudsize_x, _("X Scale clouds"), 2.0)
    description (_("Size of the clouds horizontal"))
    value_range (1.0, 4.0)
    ui_range    (1.0, 4.0)
  ui_steps      (0.01, 0.50)
  ui_meta       ("axis", "x")
    ui_meta     ("unit", "pixel-distance")
    ui_meta ("visible", "cloud_type {realistic, night}")

property_double (cloudsize_y, _("Y Scale clouds"), 2.0)
    description (_("Size of the clouds vertical"))
    value_range (1.0, 4.0)
    ui_range    (1.0, 4.0)
  ui_steps      (0.01, 0.50)
  ui_meta       ("axis", "y")
    ui_meta     ("unit", "pixel-distance")
    ui_meta ("visible", "cloud_type  {realistic, night}")

property_double (tint, _("Afternoon time tint"), 0.0)
   description  (_("Mild sepia tint adjustment for a afternoon sky"))
  ui_range (0.0, 1.0)
   value_range  (0.0, 1.0)
  ui_steps      (0.01, 0.10)
    ui_meta ("visible", "cloud_type  {realistic}")


property_double (saturation, _("Saturation for the sky"), 0.6)
   description  (_("Saturation adjustment for the sky"))
  ui_range (0.5, 1.0)
   value_range  (0.5, 1.0)
  ui_steps      (0.01, 0.10)
    ui_meta ("visible", "cloud_type  {original}")

property_double (darknessclouds, _("Darkness hides clouds"), 1.6)
   description  (_("The more dark it is the less clouds are visible,  higher values = less clouds, lower values = more clouds"))
  ui_range (1.0, 2.0)
   value_range  (1.0, 2.0)
  ui_steps      (0.01, 0.50)
    ui_meta ("visible", "cloud_type  {night}")


property_double (hue, _("Hue Rotation (0 resets)"), 0)
   description  (_("Hue rotation for non-realistic clouds"))
  ui_range (-180, 180)
   value_range  (-180, 180)
  ui_steps      (0.01, 0.50)
    ui_meta ("visible", "cloud_type  {original}")

property_seed (starseed, _("Random seed of stars"), rand)
    ui_meta ("visible", "cloud_type  {night}")

property_double (staropacity, _("Star visbility"), 0.7)
   description  (_("Visibility of the stars"))
  ui_range (0.0, 1.0)
   value_range  (0.0, 1.0)
  ui_steps      (0.01, 0.50)
    ui_meta ("visible", "cloud_type  {night}")

property_double (starlight, _("Star lighting"), 25.0)
   description  (_("Star lighting adjustment, also tweaks the size "))
  ui_range (20.0, 35.0)
   value_range  (20.0, 35.0)
  ui_steps      (0.01, 0.50)
    ui_meta ("visible", "cloud_type  {night}")



property_seed (seed, _("Random seed of clouds"), randyfineisevil)



#else

#define GEGL_OP_META
#define GEGL_OP_NAME     clouds
#define GEGL_OP_C_SOURCE clouds.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *output;
  GeglNode *graph1;
  GeglNode *graph2;
  GeglNode *graph3;
  GeglNode *gaus;
  GeglNode *noise;
  GeglNode *noise2;
  GeglNode *internallight;
  GeglNode *opacity;
  GeglNode *opacity2;
  GeglNode *normal;
  GeglNode *idref;
  GeglNode *clip;
  GeglNode *crop; 
  GeglNode *crop2;
  GeglNode *over;
  GeglNode *saturation;
  GeglNode *saturation2;
  GeglNode *hue;
  GeglNode *repairgeglgraph;
  GeglNode *sepiatint;
  GeglNode *overlay;
  GeglNode *invert;
  GeglNode *graph4;  
  GeglNode *nightgamma; 
  GeglNode *nightsaturation3; 
  GeglNode *nightidref;  
  GeglNode *nightidref2; 
  GeglNode *nightnoise;  
  GeglNode *nightinvert; 
  GeglNode *nightopacity; 
  GeglNode *nightlevels;  
  GeglNode *nightblur;
  GeglNode *nightcrop; 
  GeglNode *nightreplace; 
  GeglNode *nightnoisecloud; 
  GeglNode *nightcolor;
  GeglNode *nightlight;
  GeglNode *screenstar;  
  GeglNode *cropconnect;  
  GeglNode *containall;  
}State;


static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  State *state = o->user_data = g_malloc0 (sizeof (State));
  GeglColor *starcolor = gegl_color_new ("#ffffff");
#define STRINGGIVEN1 \
"   hue-chroma lightness=-22 value-invert hard-light aux=[ color  value=#c3f2ff  ] opacity value=4 hard-light aux=[color  high-pass ] levels out-low=0.1   "\



#define STRINGGIVEN2 \
" id=1 src aux=[ ref=1 noise-solid x-size=4.0 y-size=4.0 seed=33 emboss depth=2 gimp:layer-mode layer-mode=hsl-color composite-mode=auto aux=[ color value=#00d0ff  ] gimp:layer-mode layer-mode=addition opacity=0.08 composite-mode=auto aux=[ noise-solid x-size=3 y-size=3  ] ] crop aux=[ ref=1 ]  "\


#define STRINGGIVEN3 \
"  id=1 difference srgb=true  aux=[ ref=1  edge algorithm=sobel  opacity value=0.26 ]  "\


#define STRINGGIVEN4 \
"  id=1 gimp:layer-mode layer-mode=luminance composite-mode=auto aux=[ ref=1  hue-chroma lightness=-13 ] divide value=2.9 multiply value=3 gimp:layer-mode layer-mode=addition composite-mode=auto aux=[ color value=#0a0065 ]   "\


      state->input    = gegl_node_get_input_proxy (gegl, "input");
      state->output   = gegl_node_get_output_proxy (gegl, "output");


  state->noise    = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-solid",
   "detail", 10,    "tileable", TRUE,
                                  NULL);


   state->graph1   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string",  STRINGGIVEN1,
                                  NULL);

   state->saturation   = gegl_node_new_child (gegl,
                                  "operation", "gegl:saturation",
                                  NULL);

 state->over   = gegl_node_new_child (gegl,
                                  "operation", "gegl:over",
                                  NULL);

 state->clip   = gegl_node_new_child (gegl,
                                  "operation", "gegl:rgb-clip",
                                  NULL);

 state->crop   = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

 state->hue   = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);


  state->repairgeglgraph      = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "radius",       0,
                                         NULL);
/*2*/

state->noise2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-solid",
   "detail", 4,   
                                  NULL);


state->graph2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string",  STRINGGIVEN2,
                                  NULL);


state->graph3   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string",  STRINGGIVEN3,
                                  NULL);



state->saturation2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:saturation", "scale", 0.5,
                                  NULL);

state->invert   = gegl_node_new_child (gegl,
                                  "operation", "gegl:invert-gamma",
                                  NULL);

state->opacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 2.1, 
                                  NULL);

state->gaus   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur", "std-dev-x", 1.0, "std-dev-y", 1.0,
                                  NULL);
state->internallight   = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply", "value", 1.5,
                                  NULL);

state->opacity2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 0.35,
                                  NULL);

state->sepiatint   = gegl_node_new_child (gegl,
                                  "operation", "gegl:sepia", 
                                  NULL);

state->idref   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop", 
                                  NULL);

state->normal   = gegl_node_new_child (gegl,
                                  "operation", "gegl:over", 
                                  NULL);

state->crop2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop", 
                                  NULL);

state->overlay   = gegl_node_new_child (gegl,
                                  "operation", "gimp:layer-mode", "layer-mode", 23, "composite-mode", 0, 
                                  NULL);


state->repairgeglgraph      = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "radius",       0,
                                         NULL);

/*3*/

state->graph4   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string",  STRINGGIVEN4,
                                  NULL);

state->nightgamma   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gamma",  "value", 1.8, 
                                  NULL);

state->nightsaturation3   = gegl_node_new_child (gegl,
                                  "operation", "gegl:saturation",  "scale", 0.7, 
                                  NULL);

state->nightidref   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop", 
                                  NULL);

state->nightidref2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop", 
                                  NULL);

state->nightnoise = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-hsv", "holdness", 8, "value-distance", 0.055,  "saturation-distance", 0.0,
                                  NULL);

state->nightnoisecloud    = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-solid",
   "detail", 4,   
                                  NULL);


      state->nightopacity = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 0.7,
                                  NULL);


      state->nightlight = gegl_node_new_child (gegl,
                                  "operation", "gegl:gamma", "value", 1.6,
                                  NULL);

      state->nightinvert = gegl_node_new_child (gegl,
                                  "operation", "gegl:invert",
                                  NULL);

    state->nightlevels = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels", "in-high", 0.33, "out-high", 4.15,
                                  NULL);

  state->nightblur = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur", "std-dev-x", 1.0, "std-dev-y", 1.0,
                                  NULL);


  state->nightcrop = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

  state->cropconnect = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

  state->screenstar = gegl_node_new_child (gegl,
                                  "operation", "gegl:screen",
                                  NULL);

  state->nightreplace = gegl_node_new_child (gegl,
                                  "operation", "gegl:src",
                                  NULL);

  state->containall = gegl_node_new_child (gegl,
                                  "operation", "gegl:src",
                                  NULL);

  state->nightcolor = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                   "value", starcolor, NULL);
       
/*original*/
  gegl_operation_meta_redirect (operation, "cloudsize", state->noise, "x-size");
  gegl_operation_meta_redirect (operation, "cloudsize", state->noise, "y-size");
  gegl_operation_meta_redirect (operation, "seed", state->noise, "seed");
  gegl_operation_meta_redirect (operation, "saturation", state->saturation, "scale");
  gegl_operation_meta_redirect (operation, "hue", state->hue, "hue");

/*realistic*/
  gegl_operation_meta_redirect (operation, "cloudsize_x", state->noise2, "x-size");
  gegl_operation_meta_redirect (operation, "cloudsize_y", state->noise2, "y-size");
  gegl_operation_meta_redirect (operation, "seed", state->noise2, "seed");
  gegl_operation_meta_redirect (operation, "tint", state->sepiatint, "scale");

/*nightime*/
  gegl_operation_meta_redirect (operation, "starseed", state->nightnoise, "seed");
  gegl_operation_meta_redirect (operation, "staropacity", state->nightopacity, "value");
  gegl_operation_meta_redirect (operation, "starlight", state->nightgamma, "value");
  gegl_operation_meta_redirect (operation, "darknessclouds", state->nightlight, "value");
  gegl_operation_meta_redirect (operation, "cloudsize_x", state->nightnoisecloud, "x-size");
  gegl_operation_meta_redirect (operation, "cloudsize_y", state->nightnoisecloud, "y-size");
  gegl_operation_meta_redirect (operation, "seed", state->nightnoisecloud, "seed");

}


static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

switch (o->cloud_type) {
        break;
    case original:
  gegl_node_link_many (state->input, state->over, state->graph1, state->clip, state->crop, state->saturation, state->hue, state->repairgeglgraph, state->output, NULL);
  gegl_node_connect (state->over, "aux", state->noise, "output");
  gegl_node_connect (state->crop, "aux", state->input, "output");
        break;
    case realistic:
  gegl_node_link_many (state->input, state->graph2, state->overlay, state->gaus, state->saturation2,  state->graph3,  state->idref, state->normal, state->crop2,  state->repairgeglgraph, state->output, NULL);
  gegl_node_link_many (state->noise2, state->invert, state->internallight, state->opacity, NULL);
  gegl_node_connect (state->crop2, "aux", state->input, "output");
  gegl_node_connect (state->overlay, "aux", state->opacity, "output");
  gegl_node_connect (state->normal, "aux", state->opacity2, "output");
  gegl_node_link_many (state->idref, state->sepiatint, state->opacity2, NULL);
        break;
    case night:
  gegl_node_link_many (state->input, state->containall,  state->output, NULL);
  gegl_node_connect (state->containall, "aux", state->screenstar, "output");
  gegl_node_link_many (state->nightnoisecloud, state->cropconnect, state->graph4, state->nightlight, state->nightsaturation3, state->nightidref, state->screenstar, NULL);
  gegl_node_connect (state->screenstar, "aux", state->nightopacity, "output");
  gegl_node_link_many (state->nightidref, state->nightidref2, state->nightreplace, state->nightcrop, state->nightcolor, state->nightnoise, state->nightinvert, state->nightlevels, state->nightgamma, state->nightblur, state->nightopacity, NULL);
  gegl_node_connect (state->nightreplace, "aux", state->nightidref2, "output");
  gegl_node_connect (state->nightcrop, "aux", state->nightidref, "output");
  gegl_node_connect (state->cropconnect, "aux", state->input, "output");
        break;
    }
  }


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:clouds",
    "title",       _("Clouds"),
    "reference-hash", "b5ecl56oau1dgxsfge77g0laefe2g4f1b2ac",
    "description", _("Renders a sky of clouds."
                     ""),
    "gimp:menu-path", "<Image>/Filters/Render/Fun",
    "gimp:menu-label", _("Clouds..."),
    NULL);
}

#endif
