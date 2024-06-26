//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "accidental.h"
#include "barline.h"
#include "bracket.h"
#include "changeMap.h"
#include "clef.h"
#include "dynamic.h"
#include "fret.h"
#include "groups.h"
#include "layoutbreak.h"
#include "mscore.h"
#include "note.h"
#include "ottava.h"
#include "property.h"
#include "style.h"
#include "sym.h"
#include "tremolo.h"
#include "trill.h"
#include "vibrato.h"
#include "xml.h"

namespace Ms {

//---------------------------------------------------------
//   PropertyMetaData
//---------------------------------------------------------

struct PropertyMetaData {
      Pid id;                 // associated Pid
      P_TYPE type;            // associated P_TYPE
      bool link;              // link this property for linked elements
      const char* name;       // xml name of property
      const char* userName;   // user-visible name of property
      };

//
// always: propertyList[subtype].id == subtype
//
//

// keep this properties untranslatable for now until we put the same strings to all UI elements
#define DUMMY_QT_TRANSLATE_NOOP(x, y) y
static constexpr PropertyMetaData propertyList[] = {
      { Pid::SUBTYPE,                   P_TYPE::INT,            false, "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "subtype")                                       },
      { Pid::SELECTED,                  P_TYPE::BOOL,           false, "selected",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "selected")                                      },
      { Pid::GENERATED,                 P_TYPE::BOOL,           false, "generated",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "generated")                                     },
      { Pid::COLOR,                     P_TYPE::COLOR,          false, "color",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "color")                                         },
      { Pid::VISIBLE,                   P_TYPE::BOOL,           false, "visible",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "visible")                                       },
      { Pid::Z,                         P_TYPE::INT,            false, "z",                      DUMMY_QT_TRANSLATE_NOOP("propertyName", "z")                                             },
      { Pid::SMALL,                     P_TYPE::BOOL,           false, "small",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "small")                                         },
      { Pid::SHOW_COURTESY,             P_TYPE::BOOL,           false, "showCourtesySig",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "show courtesy")                                 },
      { Pid::KEYSIG_MODE,               P_TYPE::KEYMODE,        false, "keysig_mode",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "show courtesy")                                 },
      { Pid::LINE_TYPE,                 P_TYPE::INT,            false, "lineType",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "line type")                                     },
      { Pid::PITCH,                     P_TYPE::INT,            true,  "pitch",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "pitch")                                         },

      { Pid::TPC1,                      P_TYPE::INT,            true,  "tpc",                    DUMMY_QT_TRANSLATE_NOOP("propertyName", "tonal pitch class")                             },
      { Pid::TPC2,                      P_TYPE::INT,            true,  "tpc2",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "tonal pitch class")                             },
      { Pid::LINE,                      P_TYPE::INT,            false, "line",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "line")                                          },
      { Pid::FIXED,                     P_TYPE::BOOL,           false, "fixed",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "fixed")                                         },
      { Pid::FIXED_LINE,                P_TYPE::INT,            false, "fixedLine",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "fixed line")                                    },
      { Pid::HEAD_TYPE,                 P_TYPE::HEAD_TYPE,      false, "headType",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "head type")                                     },
      { Pid::HEAD_GROUP,                P_TYPE::HEAD_GROUP,     false, "head",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "head")                                          },
      { Pid::VELO_TYPE,                 P_TYPE::VALUE_TYPE,     false, "veloType",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "velocity type")                                 },
      { Pid::VELO_OFFSET,               P_TYPE::INT,            false, "velocity",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "velocity")                                      },
      { Pid::ARTICULATION_ANCHOR,       P_TYPE::INT,            false, "anchor",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "anchor")                                        },

      { Pid::DIRECTION,                 P_TYPE::DIRECTION,      false, "direction",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "direction")                                     },
      { Pid::STEM_DIRECTION,            P_TYPE::DIRECTION,      false, "StemDirection",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "stem direction")                                },
      { Pid::NO_STEM,                   P_TYPE::BOOL,           false, "noStem",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "no stem")                                       },
      { Pid::SLUR_DIRECTION,            P_TYPE::DIRECTION,      false, "up",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "up")                                            },
      { Pid::LEADING_SPACE,             P_TYPE::SPATIUM,        false, "leadingSpace",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "leading space")                                 },
      { Pid::DISTRIBUTE,                P_TYPE::BOOL,           false, "distribute",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "distributed")                                   },
      { Pid::MIRROR_HEAD,               P_TYPE::DIRECTION_H,    false, "mirror",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "mirror")                                        },
      { Pid::DOT_POSITION,              P_TYPE::DIRECTION,      false, "dotPosition",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "dot position")                                  },
      { Pid::TUNING,                    P_TYPE::REAL,           false, "tuning",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "tuning")                                        },
      { Pid::PAUSE,                     P_TYPE::REAL,           true,  "pause",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "pause")                                         },

      { Pid::BARLINE_TYPE,              P_TYPE::BARLINE_TYPE,   false, "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "subtype")                                       },
      { Pid::BARLINE_SPAN,              P_TYPE::BOOL,           false, "span",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "span")                                          },
      { Pid::BARLINE_SPAN_FROM,         P_TYPE::INT,            false, "spanFromOffset",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "span from")                                     },
      { Pid::BARLINE_SPAN_TO,           P_TYPE::INT,            false, "spanToOffset",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "span to")                                       },
      { Pid::OFFSET,                    P_TYPE::POINT_SP_MM,    false, "offset",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "offset")                                        },
      { Pid::FRET,                      P_TYPE::INT,            true,  "fret",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "fret")                                          },
      { Pid::STRING,                    P_TYPE::INT,            true,  "string",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "string")                                        },
      { Pid::GHOST,                     P_TYPE::BOOL,           true,  "ghost",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "ghost")                                         },
      { Pid::PLAY,                      P_TYPE::BOOL,           false, "play",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "played")                                        },
      { Pid::TIMESIG_NOMINAL,           P_TYPE::FRACTION,       false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "nominal time signature")                        },

      { Pid::TIMESIG_ACTUAL,            P_TYPE::FRACTION,       true,  0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "actual time signature")                         },
      { Pid::NUMBER_TYPE,               P_TYPE::INT,            false, "numberType",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "number type")                                   },
      { Pid::BRACKET_TYPE,              P_TYPE::INT,            false, "bracketType",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "bracket type")                                  },
      { Pid::NORMAL_NOTES,              P_TYPE::INT,            false, "normalNotes",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "normal notes")                                  },
      { Pid::ACTUAL_NOTES,              P_TYPE::INT,            false, "actualNotes",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "actual notes")                                  },
      { Pid::P1,                        P_TYPE::POINT_SP,       false, "p1",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "p1")                                            },
      { Pid::P2,                        P_TYPE::POINT_SP,       false, "p2",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "p2")                                            },
      { Pid::GROW_LEFT,                 P_TYPE::REAL,           false, "growLeft",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "grow left")                                     },
      { Pid::GROW_RIGHT,                P_TYPE::REAL,           false, "growRight",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "grow right")                                    },
      { Pid::BOX_HEIGHT,                P_TYPE::SPATIUM,        false, "height",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "height")                                        },

      { Pid::BOX_WIDTH,                 P_TYPE::SPATIUM,        false, "width",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "width")                                         },
      { Pid::BOX_AUTOSIZE,              P_TYPE::BOOL,           false, "boxAutoSize",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "autosize frame")                                },
      { Pid::TOP_GAP,                   P_TYPE::SP_REAL,        false, "topGap",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "top gap")                                       },
      { Pid::BOTTOM_GAP,                P_TYPE::SP_REAL,        false, "bottomGap",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "bottom gap")                                    },
      { Pid::LEFT_MARGIN,               P_TYPE::REAL,           false, "leftMargin",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "left margin")                                   },
      { Pid::RIGHT_MARGIN,              P_TYPE::REAL,           false, "rightMargin",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "right margin")                                  },
      { Pid::TOP_MARGIN,                P_TYPE::REAL,           false, "topMargin",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "top margin")                                    },
      { Pid::BOTTOM_MARGIN,             P_TYPE::REAL,           false, "bottomMargin",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "bottom margin")                                 },
      { Pid::LAYOUT_BREAK,              P_TYPE::LAYOUT_BREAK,   false, "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "subtype")                                       },
      { Pid::AUTOSCALE,                 P_TYPE::BOOL,           false, "autoScale",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "autoscale")                                     },
      { Pid::SIZE,                      P_TYPE::SIZE,           false, "size",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "size")                                          },

      { Pid::SCALE,                     P_TYPE::SCALE,          false, "scale",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "scale")                                         },
      { Pid::LOCK_ASPECT_RATIO,         P_TYPE::BOOL,           false, "lockAspectRatio",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "aspect ratio locked")                           },
      { Pid::SIZE_IS_SPATIUM,           P_TYPE::BOOL,           false, "sizeIsSpatium",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "size is spatium")                               },
      { Pid::TEXT,                      P_TYPE::STRING,         true,  "text",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "text")                                          },
      { Pid::HTML_TEXT,                 P_TYPE::STRING,         false, 0,                        ""                                                                                       },
      { Pid::USER_MODIFIED,             P_TYPE::BOOL,           false, 0,                        ""                                                                                       },
      { Pid::BEAM_POS,                  P_TYPE::POINT,          false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "beam position")                                 },
      { Pid::BEAM_MODE,                 P_TYPE::BEAM_MODE,      true, "BeamMode",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "beam mode")                                     },
      { Pid::BEAM_NO_SLOPE,             P_TYPE::BOOL,           true, "noSlope",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "without slope")                                 },
      { Pid::USER_LEN,                  P_TYPE::SP_REAL,        false, "userLen",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "length")                                        },

      { Pid::SPACE,                     P_TYPE::SP_REAL,        false, "space",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "space")                                         },
      { Pid::TEMPO,                     P_TYPE::TEMPO,          true,  "tempo",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "tempo")                                         },
      { Pid::TEMPO_FOLLOW_TEXT,         P_TYPE::BOOL,           true,  "followText",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "following text")                                },
      { Pid::ACCIDENTAL_BRACKET,        P_TYPE::INT,            false, "bracket",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "bracket")                                       },
      { Pid::ACCIDENTAL_TYPE,           P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "type")                                          },
      { Pid::NUMERATOR_STRING,          P_TYPE::STRING,         false, "textN",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "numerator string")                              },
      { Pid::DENOMINATOR_STRING,        P_TYPE::STRING,         false, "textD",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "denominator string")                            },
      { Pid::FBPREFIX,                  P_TYPE::INT,            false, "prefix",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "prefix")                                        },
      { Pid::FBDIGIT,                   P_TYPE::INT,            false, "digit",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "digit")                                         },
      { Pid::FBSUFFIX,                  P_TYPE::INT,            false, "suffix",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "suffix")                                        },
      { Pid::FBCONTINUATIONLINE,        P_TYPE::INT,            false, "continuationLine",       DUMMY_QT_TRANSLATE_NOOP("propertyName", "continuation line")                             },

      { Pid::FBPARENTHESIS1,            P_TYPE::INT,            false, "",                       ""                                                                                       },
      { Pid::FBPARENTHESIS2,            P_TYPE::INT,            false, "",                       ""                                                                                       },
      { Pid::FBPARENTHESIS3,            P_TYPE::INT,            false, "",                       ""                                                                                       },
      { Pid::FBPARENTHESIS4,            P_TYPE::INT,            false, "",                       ""                                                                                       },
      { Pid::FBPARENTHESIS5,            P_TYPE::INT,            false, "",                       ""                                                                                       },
      { Pid::OTTAVA_TYPE,               P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "ottava type")                                   },
      { Pid::NUMBERS_ONLY,              P_TYPE::BOOL,           false, "numbersOnly",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "numbers only")                                  },
      { Pid::TRILL_TYPE,                P_TYPE::INT,            false, "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "trill type")                                    },
      { Pid::VIBRATO_TYPE,              P_TYPE::INT,            false, "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "vibrato type")                                  },
      { Pid::HAIRPIN_CIRCLEDTIP,        P_TYPE::BOOL,           false, "hairpinCircledTip",      DUMMY_QT_TRANSLATE_NOOP("propertyName", "hairpin with circled tip")                      },

      { Pid::HAIRPIN_TYPE,              P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "hairpin type")                                  },
      { Pid::HAIRPIN_HEIGHT,            P_TYPE::SPATIUM,        false, "hairpinHeight",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "hairpin height")                                },
      { Pid::HAIRPIN_CONT_HEIGHT,       P_TYPE::SPATIUM,        false, "hairpinContHeight",      DUMMY_QT_TRANSLATE_NOOP("propertyName", "hairpin cont height")                           },
      { Pid::VELO_CHANGE,               P_TYPE::INT,            true,  "veloChange",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "velocity change")                               },
      { Pid::VELO_CHANGE_METHOD,        P_TYPE::CHANGE_METHOD,  true,  "veloChangeMethod",       DUMMY_QT_TRANSLATE_NOOP("propertyName", "velocity change method")                        },     // left as a compatability property - we need to be able to read it correctly
      { Pid::VELO_CHANGE_SPEED,         P_TYPE::CHANGE_SPEED,   true,  "veloChangeSpeed",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "velocity change speed")                         },
      { Pid::DYNAMIC_TYPE,              P_TYPE::DYNAMIC_TYPE,   true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "dynamic type")                                  },
      { Pid::DYNAMIC_RANGE,             P_TYPE::INT,            true,  "dynType",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "dynamic range")                                 },
//100
      { Pid::SINGLE_NOTE_DYNAMICS,      P_TYPE::BOOL,           true,  "singleNoteDynamics",     DUMMY_QT_TRANSLATE_NOOP("propertyName", "single note dynamics")                          },
      { Pid::CHANGE_METHOD,             P_TYPE::CHANGE_METHOD,  true,  "changeMethod",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "change method")                                 },        // the new, more general version of VELO_CHANGE_METHOD
      { Pid::PLACEMENT,                 P_TYPE::PLACEMENT,      false, "placement",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "placement")                                     },
      { Pid::HPLACEMENT,                P_TYPE::HPLACEMENT,     false, "hplacement",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "horizontal placement")                          },
      { Pid::MMREST_RANGE_BRACKET_TYPE, P_TYPE::INT,            false, "mmrestRangeBracketType", DUMMY_QT_TRANSLATE_NOOP("propertyName", "multimeasure rest range bracket type")          },
      { Pid::VELOCITY,                  P_TYPE::INT,            false, "velocity",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "velocity")                                      },
      { Pid::JUMP_TO,                   P_TYPE::STRING,         true,  "jumpTo",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "jump to")                                       },
      { Pid::PLAY_UNTIL,                P_TYPE::STRING,         true,  "playUntil",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "play until")                                    },
      { Pid::CONTINUE_AT,               P_TYPE::STRING,         true,  "continueAt",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue at")                                   },
      { Pid::LABEL,                     P_TYPE::STRING,         true,  "label",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "label")                                         },
      { Pid::MARKER_TYPE,               P_TYPE::INT,            true,  0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "marker type")                                   },
      { Pid::ARP_USER_LEN1,             P_TYPE::REAL,           false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "length 1")                                      },
      { Pid::ARP_USER_LEN2,             P_TYPE::REAL,           false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "length 2")                                      },
      { Pid::REPEAT_END,                P_TYPE::BOOL,           true,  0,                        ""                                                                                       },
      { Pid::REPEAT_START,              P_TYPE::BOOL,           true,  0,                        ""                                                                                       },
      { Pid::REPEAT_JUMP,               P_TYPE::BOOL,           true,  0,                        ""                                                                                       },
      { Pid::MEASURE_NUMBER_MODE,       P_TYPE::INT,            false, "measureNumberMode",      DUMMY_QT_TRANSLATE_NOOP("propertyName", "measure number mode")                           },

      { Pid::GLISS_TYPE,                P_TYPE::INT,            false, "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "subtype")                                       },
      { Pid::GLISS_TEXT,                P_TYPE::STRING,         false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "text")                                          },
      { Pid::GLISS_SHOW_TEXT,           P_TYPE::BOOL,           false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "showing text")                                  },
      { Pid::GLISS_STYLE,               P_TYPE::GLISS_STYLE,    true,  "glissandoStyle",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "glissando style")                               },
      { Pid::GLISS_EASEIN,              P_TYPE::INT,            false, "easeInSpin",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "ease in")                                       },
      { Pid::GLISS_EASEOUT,             P_TYPE::INT,            false, "easeOutSpin",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "ease out")                                      },
      { Pid::DIAGONAL,                  P_TYPE::BOOL,           false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "diagonal")                                      },
      { Pid::GROUPS,                    P_TYPE::GROUPS,         false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "groups")                                        },
      { Pid::LINE_STYLE,                P_TYPE::INT,            true,  "lineStyle",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "line style")                                    },
      { Pid::LINE_WIDTH,                P_TYPE::SP_REAL,        false, "lineWidth",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "line width")                                    },
      { Pid::LINE_WIDTH_SPATIUM,        P_TYPE::SPATIUM,        false, "lineWidth",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "line width (spatium)")                          },
      { Pid::LASSO_POS,                 P_TYPE::POINT_MM,       false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "lasso position")                                },
      { Pid::LASSO_SIZE,                P_TYPE::SIZE_MM,        false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "lasso size")                                    },
      { Pid::TIME_STRETCH,              P_TYPE::REAL,           true,  "timeStretch",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "time stretch")                                  },
      { Pid::ORNAMENT_STYLE,            P_TYPE::ORNAMENT_STYLE, true,  "ornamentStyle",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "ornament style")                                },

      { Pid::TIMESIG,                   P_TYPE::FRACTION,       false, "timesig",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "time signature")                                },
      { Pid::TIMESIG_STRETCH,           P_TYPE::FRACTION,       false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "time signature stretch")                        },
      { Pid::TIMESIG_TYPE,              P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "subtype")                                       },
      { Pid::SPANNER_TICK,              P_TYPE::FRACTION,       true,  "tick",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "tick")                                          },
      { Pid::SPANNER_TICKS,             P_TYPE::FRACTION,       true,  "ticks",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "ticks")                                         },
      { Pid::SPANNER_TRACK2,            P_TYPE::INT,            false, "track2",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "track2")                                        },
      { Pid::OFFSET2,                   P_TYPE::POINT_SP,       false, "userOff2",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "offset2")                                       },
      { Pid::BREAK_MMR,                 P_TYPE::BOOL,           false, "breakMultiMeasureRest",  DUMMY_QT_TRANSLATE_NOOP("propertyName", "breaking multimeasure rest") },
      { Pid::MMREST_NUMBER_POS,         P_TYPE::SPATIUM,        false, "mmRestNumberPos",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "vertical position of multimeasure rest number") },
      //{ Pid::MMREST_NUMBER_VISIBLE,     P_TYPE::BOOL,           false, "mmRestNumberVisible",    DUMMY_QT_TRANSLATE_NOOP("propertyName", "visibility of multimeasure rest number") },
      { Pid::MEASURE_REPEAT_NUMBER_POS, P_TYPE::SPATIUM,        false, "measureRepeatNumberPos", DUMMY_QT_TRANSLATE_NOOP("propertyName", "vertical position of measure repeat number") },
      { Pid::REPEAT_COUNT,              P_TYPE::INT,            true,  "endRepeat",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "end repeat")                                    },

      { Pid::USER_STRETCH,              P_TYPE::REAL,           false, "stretch",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "stretch")                                       },
      { Pid::NO_OFFSET,                 P_TYPE::INT,            true,  "noOffset",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "numbering offset")                              },
      { Pid::IRREGULAR,                 P_TYPE::BOOL,           true,  "irregular",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "irregular")                                     },
      { Pid::ANCHOR,                    P_TYPE::INT,            false, "anchor",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "anchor")                                        },
      { Pid::SLUR_UOFF1,                P_TYPE::POINT_SP,       false, "o1",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "o1")                                            },
      { Pid::SLUR_UOFF2,                P_TYPE::POINT_SP,       false, "o2",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "o2")                                            },
      { Pid::SLUR_UOFF3,                P_TYPE::POINT_SP,       false, "o3",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "o3")                                            },
      { Pid::SLUR_UOFF4,                P_TYPE::POINT_SP,       false, "o4",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "o4")                                            },
      { Pid::STAFF_MOVE,                P_TYPE::INT,            true,  "staffMove",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "staff move")                                    },
      { Pid::VERSE,                     P_TYPE::ZERO_INT,       true,  "no",                     DUMMY_QT_TRANSLATE_NOOP("propertyName", "verse")                                         },

      { Pid::SYLLABIC,                  P_TYPE::INT,            true,  "syllabic",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "syllabic")                                      },
      { Pid::LYRIC_TICKS,               P_TYPE::FRACTION,       true,  "ticks_f",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "ticks")                                         },
      { Pid::VOLTA_ENDING,              P_TYPE::INT_LIST,       true,  "endings",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "endings")                                       },
      { Pid::LINE_VISIBLE,              P_TYPE::BOOL,           true,  "lineVisible",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "visible line")                                  },
      { Pid::MAG,                       P_TYPE::REAL,           false, "mag",                    DUMMY_QT_TRANSLATE_NOOP("propertyName", "mag")                                           },
      { Pid::USE_DRUMSET,               P_TYPE::BOOL,           false, "useDrumset",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "using drumset")                                 },
      { Pid::DURATION,                  P_TYPE::FRACTION,       true,  0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "duration")                                      },
      { Pid::DURATION_TYPE,             P_TYPE::TDURATION,      true,  0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "duration type")                                 },
      { Pid::ROLE,                      P_TYPE::INT,            false, "role",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "role")                                          },
      { Pid::TRACK,                     P_TYPE::INT,            false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "track")                                         },

      { Pid::FRET_STRINGS,              P_TYPE::INT,            true,  "strings",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "strings")                                       },
      { Pid::FRET_FRETS,                P_TYPE::INT,            true,  "frets",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "frets")                                         },
      { Pid::FRET_NUT,                  P_TYPE::BOOL,           true,  "showNut",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "show nut")                                      },
      { Pid::FRET_OFFSET,               P_TYPE::INT,            true,  "fretOffset",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "fret offset")                                   },
      { Pid::FRET_NUM_POS,              P_TYPE::INT,            true,  "fretNumPos",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "fret number position")                          },
      { Pid::ORIENTATION,               P_TYPE::ORIENTATION,    true,  "orientation",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "orientation")                                   },

      { Pid::HARMONY_VOICE_LITERAL,     P_TYPE::BOOL,           true,  "harmonyVoiceLiteral",    DUMMY_QT_TRANSLATE_NOOP("propertyName", "harmony voice literal")                         },
      { Pid::HARMONY_VOICING,           P_TYPE::INT,            true,  "harmonyVoicing",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "harmony voicing")                               },
      { Pid::HARMONY_DURATION,          P_TYPE::INT,            true,  "harmonyDuration",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "harmony duration")                              },

      { Pid::SYSTEM_BRACKET,            P_TYPE::INT,            false, "type",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "type")                                          },
      { Pid::GAP,                       P_TYPE::BOOL,           false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "gap")                                           },
      { Pid::AUTOPLACE,                 P_TYPE::BOOL,           false, "autoplace",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "autoplace")                                     },
      { Pid::DASH_LINE_LEN,             P_TYPE::REAL,           false, "dashLineLength",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "dash line length")                              },
      { Pid::DASH_GAP_LEN,              P_TYPE::REAL,           false, "dashGapLength",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "dash gap length")                               },
      { Pid::TICK,                      P_TYPE::FRACTION,       false, 0,                        DUMMY_QT_TRANSLATE_NOOP("propertyName", "tick")                                          },
      { Pid::PLAYBACK_VOICE1,           P_TYPE::BOOL,           false, "playbackVoice1",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "playback voice 1")                              },
      { Pid::PLAYBACK_VOICE2,           P_TYPE::BOOL,           false, "playbackVoice2",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "playback voice 2")                              },
      { Pid::PLAYBACK_VOICE3,           P_TYPE::BOOL,           false, "playbackVoice3",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "playback voice 3")                              },

      { Pid::PLAYBACK_VOICE4,           P_TYPE::BOOL,           false, "playbackVoice4",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "playback voice 4")                              },
      { Pid::SYMBOL,                    P_TYPE::SYMID,          true,  "symbol",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "symbol")                                        },
      { Pid::PLAY_REPEATS,              P_TYPE::BOOL,           true,  "playRepeats",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "playing repeats")                               },
      { Pid::CREATE_SYSTEM_HEADER,      P_TYPE::BOOL,           false, "createSystemHeader",     DUMMY_QT_TRANSLATE_NOOP("propertyName", "creating system header")                        },
      { Pid::STAFF_LINES,               P_TYPE::INT,            true,  "lines",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "lines")                                         },
      { Pid::LINE_DISTANCE,             P_TYPE::SPATIUM,        true,  "lineDistance",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "line distance")                                 },
      { Pid::STEP_OFFSET,               P_TYPE::INT,            true,  "stepOffset",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "step offset")                                   },
      { Pid::STAFF_SHOW_BARLINES,       P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "showing barlines")                              },
      { Pid::STAFF_SHOW_LEDGERLINES,    P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "showing ledgerlines")                           },
      { Pid::STAFF_STEMLESS,            P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "stemless")                                      },
      { Pid::STAFF_INVISIBLE,           P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "invisible")                                     },
      { Pid::STAFF_COLOR,               P_TYPE::COLOR,          false, "color",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "color")                                         },

      { Pid::HEAD_SCHEME,               P_TYPE::HEAD_SCHEME,    false, "headScheme",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "notehead scheme")                               },
      { Pid::STAFF_GEN_CLEF,            P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "generating clefs")                              },
      { Pid::STAFF_GEN_TIMESIG,         P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "generating time signature")                     },
      { Pid::STAFF_GEN_KEYSIG,          P_TYPE::BOOL,           false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "generating key signature")                      },
      { Pid::STAFF_YOFFSET,             P_TYPE::SPATIUM,        false, "",                       DUMMY_QT_TRANSLATE_NOOP("propertyName", "y-offset")                                      },
      { Pid::STAFF_USERDIST,            P_TYPE::SP_REAL,        false, "distOffset",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "distance offset")                               },
      { Pid::STAFF_BARLINE_SPAN,        P_TYPE::BOOL,           false, "barLineSpan",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "barline span")                                  },
      { Pid::STAFF_BARLINE_SPAN_FROM,   P_TYPE::INT,            false, "barLineSpanFrom",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "barline span from")                             },
      { Pid::STAFF_BARLINE_SPAN_TO,     P_TYPE::INT,            false, "barLineSpanTo",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "barline span to")                               },
      { Pid::BRACKET_SPAN,              P_TYPE::INT,            false, "bracketSpan",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "bracket span")                                  },

      { Pid::BRACKET_COLUMN,            P_TYPE::INT,            false, "level",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "level")                                         },
      { Pid::INAME_LAYOUT_POSITION,     P_TYPE::INT,            false, "layoutPosition",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "layout position")                               },
//200
      { Pid::SUB_STYLE,                 P_TYPE::SUB_STYLE,      false, "style",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "style")                                         },
      { Pid::FONT_FACE,                 P_TYPE::FONT,           false, "family",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "family")                                        },
      { Pid::FONT_SIZE,                 P_TYPE::REAL,           false, "size",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "size")                                          },
      { Pid::FONT_STYLE,                P_TYPE::INT,            false, "fontStyle",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "font style")                                    },
      { Pid::TEXT_LINE_SPACING,         P_TYPE::REAL,           false, "textLineSpacing",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "user line distancing")                          },

      { Pid::FRAME_TYPE,                P_TYPE::INT,            false, "frameType",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "frame type")                                    },
      { Pid::FRAME_WIDTH,               P_TYPE::SPATIUM,        false, "frameWidth",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "frame width")                                   },
      { Pid::FRAME_PADDING,             P_TYPE::SPATIUM,        false, "framePadding",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "frame padding")                                 },
      { Pid::FRAME_ROUND,               P_TYPE::INT,            false, "frameRound",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "frame round")                                   },
      { Pid::FRAME_FG_COLOR,            P_TYPE::COLOR,          false, "frameFgColor",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "frame foreground color")                        },
      { Pid::FRAME_BG_COLOR,            P_TYPE::COLOR,          false, "frameBgColor",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "frame background color")                        },
      { Pid::SIZE_SPATIUM_DEPENDENT,    P_TYPE::BOOL,           false, "sizeIsSpatiumDependent", DUMMY_QT_TRANSLATE_NOOP("propertyName", "spatium dependent font")                        },
      { Pid::ALIGN,                     P_TYPE::ALIGN,          false, "align",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "align")                                         },
      { Pid::SYSTEM_FLAG,               P_TYPE::BOOL,           false, "systemFlag",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "system flag")                                   },
      { Pid::BEGIN_TEXT,                P_TYPE::STRING,         true,  "beginText",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin text")                                    },

      { Pid::BEGIN_TEXT_ALIGN,          P_TYPE::ALIGN,          false, "beginTextAlign",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin text align")                              },
      { Pid::BEGIN_TEXT_PLACE,          P_TYPE::TEXT_PLACE,     false, "beginTextPlace",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin text place")                              },
      { Pid::BEGIN_HOOK_TYPE,           P_TYPE::INT,            true,  "beginHookType",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin hook type")                               },
      { Pid::BEGIN_HOOK_HEIGHT,         P_TYPE::SPATIUM,        false, "beginHookHeight",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin hook height")                             },
      { Pid::BEGIN_FONT_FACE,           P_TYPE::FONT,           false, "beginFontFace",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin font face")                               },
      { Pid::BEGIN_FONT_SIZE,           P_TYPE::REAL,           false, "beginFontSize",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin font size")                               },
      { Pid::BEGIN_FONT_STYLE,          P_TYPE::INT,            false, "beginFontStyle",         DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin font style")                              },
      { Pid::BEGIN_TEXT_OFFSET,         P_TYPE::POINT_SP,       false, "beginTextOffset",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "begin text offset")                             },

      { Pid::CONTINUE_TEXT,             P_TYPE::STRING,         true,  "continueText",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue text")                                 },
      { Pid::CONTINUE_TEXT_ALIGN,       P_TYPE::ALIGN,          false, "continueTextAlign",      DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue text align")                           },
      { Pid::CONTINUE_TEXT_PLACE,       P_TYPE::TEXT_PLACE,     false, "continueTextPlace",      DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue text place")                           },
      { Pid::CONTINUE_FONT_FACE,        P_TYPE::FONT,           false, "continueFontFace",       DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue font face")                            },
      { Pid::CONTINUE_FONT_SIZE,        P_TYPE::REAL,           false, "continueFontSize",       DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue font size")                            },
      { Pid::CONTINUE_FONT_STYLE,       P_TYPE::INT,            false, "continueFontStyle",      DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue font style")                           },
      { Pid::CONTINUE_TEXT_OFFSET,      P_TYPE::POINT_SP,       false, "continueTextOffset",     DUMMY_QT_TRANSLATE_NOOP("propertyName", "continue text offset")                          },
      { Pid::END_TEXT,                  P_TYPE::STRING,         true,  "endText",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "end text")                                      },

      { Pid::END_TEXT_ALIGN,            P_TYPE::ALIGN,          false, "endTextAlign",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "end text align")                                },
      { Pid::END_TEXT_PLACE,            P_TYPE::TEXT_PLACE,     false, "endTextPlace",           DUMMY_QT_TRANSLATE_NOOP("propertyName", "end text place")                                },
      { Pid::END_HOOK_TYPE,             P_TYPE::INT,            true,  "endHookType",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "end hook type")                                 },
      { Pid::END_HOOK_HEIGHT,           P_TYPE::SPATIUM,        false, "endHookHeight",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "end hook height")                               },
      { Pid::END_FONT_FACE,             P_TYPE::FONT,           false, "endFontFace",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "end font face")                                 },
      { Pid::END_FONT_SIZE,             P_TYPE::REAL,           false, "endFontSize",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "end font size")                                 },
      { Pid::END_FONT_STYLE,            P_TYPE::INT,            false, "endFontStyle",           DUMMY_QT_TRANSLATE_NOOP("propertyName",  "end font style")                               },
      { Pid::END_TEXT_OFFSET,           P_TYPE::POINT_SP,       false, "endTextOffset",          DUMMY_QT_TRANSLATE_NOOP("propertyName", "end text offset")                               },

      { Pid::POS_ABOVE,                 P_TYPE::SP_REAL,        false, "posAbove",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "position above")                                },

      { Pid::LOCATION_STAVES,           P_TYPE::INT,            false, "staves",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "staves distance")                               },
      { Pid::LOCATION_VOICES,           P_TYPE::INT,            false, "voices",                 DUMMY_QT_TRANSLATE_NOOP("propertyName", "voices distance")                               },
      { Pid::LOCATION_MEASURES,         P_TYPE::INT,            false, "measures",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "measures distance")                             },
      { Pid::LOCATION_FRACTIONS,        P_TYPE::FRACTION,       false, "fractions",              DUMMY_QT_TRANSLATE_NOOP("propertyName", "position distance")                             },
      { Pid::LOCATION_GRACE,            P_TYPE::INT,            false, "grace",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "grace note index")                              },
      { Pid::LOCATION_NOTE,             P_TYPE::INT,            false, "note",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "note index")                                    },

      { Pid::VOICE,                     P_TYPE::INT,            true,  "voice",                  DUMMY_QT_TRANSLATE_NOOP("propertyName", "voice")                                         },
      { Pid::POSITION,                  P_TYPE::FRACTION,       false, "position",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "position")                                      },

      { Pid::CLEF_TYPE_CONCERT,         P_TYPE::CLEF_TYPE,      true,  "concertClefType",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "concert clef type")                             },
      { Pid::CLEF_TYPE_TRANSPOSING,     P_TYPE::CLEF_TYPE,      true,  "transposingClefType",    DUMMY_QT_TRANSLATE_NOOP("propertyName", "transposing clef type")                         },
      { Pid::KEY,                       P_TYPE::INT,            true,  "accidental",             DUMMY_QT_TRANSLATE_NOOP("propertyName", "key")                                           },
      { Pid::ACTION,                    P_TYPE::STRING,         false, "action",                 0                                                                                        },
      { Pid::MIN_DISTANCE,              P_TYPE::SPATIUM,        false, "minDistance",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "autoplace minimum distance")                    },

      { Pid::ARPEGGIO_TYPE,             P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "arpeggio type")                                 },
      { Pid::CHORD_LINE_TYPE,           P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "chord line type")                               },
      { Pid::CHORD_LINE_STRAIGHT,       P_TYPE::BOOL,           true,  "straight",               DUMMY_QT_TRANSLATE_NOOP("propertyName", "straight chord line")                           },
      { Pid::TREMOLO_TYPE,              P_TYPE::INT,            true,  "subtype",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "tremolo type")                                  },
      { Pid::TREMOLO_STYLE,             P_TYPE::INT,            true,  "strokeStyle",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "tremolo style")                                 },
      { Pid::HARMONY_TYPE,              P_TYPE::INT,            true,  "harmonyType",            DUMMY_QT_TRANSLATE_NOOP("propertyName", "harmony type")                                  },

      { Pid::START_WITH_LONG_NAMES,     P_TYPE::BOOL,           false, "startWithLongNames",     DUMMY_QT_TRANSLATE_NOOP("propertyName", "start with long names")                         },
      { Pid::START_WITH_MEASURE_ONE,    P_TYPE::BOOL,           true,  "startWithMeasureOne",    DUMMY_QT_TRANSLATE_NOOP("propertyName", "start with measure one")                        },
      { Pid::FIRST_SYSTEM_INDENTATION,  P_TYPE::BOOL,           true,  "firstSystemIndentation", DUMMY_QT_TRANSLATE_NOOP("propertyName", "first system indentation")                      },

      { Pid::PATH,                      P_TYPE::PATH,           false, "path",                   DUMMY_QT_TRANSLATE_NOOP("propertyName", "path")                                          },

      { Pid::PREFER_SHARP_FLAT,         P_TYPE::INT,            true,  "preferSharpFlat",        DUMMY_QT_TRANSLATE_NOOP("propertyName", "prefer sharps or flats")                        },

      { Pid::END,                       P_TYPE::INT,            false, "++end++",                DUMMY_QT_TRANSLATE_NOOP("propertyName", "<invalid property>")                            }
      };

//---------------------------------------------------------
//   propertyId
//---------------------------------------------------------

Pid propertyId(const QStringRef& s)
      {
      for (const PropertyMetaData& pd : propertyList) {
            if (pd.name == s)
                  return pd.id;
            }
      return Pid::END;
      }

//---------------------------------------------------------
//   propertyId
//---------------------------------------------------------

Pid propertyId(const QString& s)
      {
      return propertyId(QStringRef(&s));
      }

//---------------------------------------------------------
//   propertyType
//---------------------------------------------------------

P_TYPE propertyType(Pid id)
      {
      Q_ASSERT( propertyList[int(id)].id == id);
      return propertyList[int(id)].type;
      }

//---------------------------------------------------------
//   propertyLink
//---------------------------------------------------------

bool propertyLink(Pid id)
      {
      Q_ASSERT( propertyList[int(id)].id == id);
      return propertyList[int(id)].link;
      }

//---------------------------------------------------------
//   propertyName
//---------------------------------------------------------

const char* propertyName(Pid id)
      {
      Q_ASSERT( propertyList[int(id)].id == id);
      return propertyList[int(id)].name;
      }

//---------------------------------------------------------
//   propertyUserName
//---------------------------------------------------------

QString propertyUserName(Pid id)
      {
      Q_ASSERT(propertyList[int(id)].id == id);
      return QObject::tr(propertyList[int(id)].userName, "propertyName");
      }

//---------------------------------------------------------
//    propertyFromString
//---------------------------------------------------------

QVariant propertyFromString(Pid id, QString value)
      {
      switch (propertyType(id)) {
            case P_TYPE::BOOL:
                  return QVariant(bool(value.toInt()));
            case P_TYPE::ZERO_INT:
            case P_TYPE::INT:
                  return QVariant(value.toInt());
            case P_TYPE::REAL:
            case P_TYPE::SPATIUM:
            case P_TYPE::SP_REAL:
            case P_TYPE::TEMPO:
                  return QVariant(value.toDouble());
            case P_TYPE::FRACTION:
                  return Fraction::fromString(value);
            case P_TYPE::COLOR:
                  // not used by MSCX
                  return QColor(value);
            case P_TYPE::POINT:
            case P_TYPE::POINT_SP:
            case P_TYPE::POINT_SP_MM: {
                  // not used by MSCX
                  const int i = value.indexOf(';');
                  return QPointF(value.leftRef(i).toDouble(), value.midRef(i+1).toDouble());
                  }
            case P_TYPE::SCALE:
            case P_TYPE::SIZE: {
                  // not used by MSCX
                  const int i = value.indexOf('x');
                  return QSizeF(value.leftRef(i).toDouble(), value.midRef(i+1).toDouble());
                  }
            case P_TYPE::FONT:
            case P_TYPE::STRING:
                  return value;
            case P_TYPE::GLISS_STYLE: {
                  if ( value == "whitekeys")
                        return QVariant(int(GlissandoStyle::WHITE_KEYS));
                  else if ( value == "blackkeys")
                        return QVariant(int(GlissandoStyle::BLACK_KEYS));
                  else if ( value == "diatonic")
                        return QVariant(int(GlissandoStyle::DIATONIC));
                  else if ( value == "portamento")
                        return QVariant(int(GlissandoStyle::PORTAMENTO));
                  else // e.g., normally "Chromatic"
                        return QVariant(int(GlissandoStyle::CHROMATIC));
                  }
                  break;
            case P_TYPE::ORNAMENT_STYLE: {
                  if ( value == "baroque")
                        return QVariant(int(MScore::OrnamentStyle::BAROQUE));
                  return QVariant(int(MScore::OrnamentStyle::DEFAULT));
                  }

            case P_TYPE::DIRECTION:
                  return QVariant::fromValue<Direction>(toDirection(value));

            case P_TYPE::DIRECTION_H:
                  {
                  if (value == "left" || value == "1")
                        return QVariant(int(MScore::DirectionH::LEFT));
                  else if (value == "right" || value == "2")
                        return QVariant(int(MScore::DirectionH::RIGHT));
                  else if (value == "auto")
                        return QVariant(int(MScore::DirectionH::AUTO));
                  }
                  break;
            case P_TYPE::LAYOUT_BREAK: {
                  if (value == "line")
                        return QVariant(int(LayoutBreak::LINE));
                  if (value == "page")
                        return QVariant(int(LayoutBreak::PAGE));
                  if (value == "section")
                        return QVariant(int(LayoutBreak::SECTION));
                  if (value == "nobreak")
                        return QVariant(int(LayoutBreak::NOBREAK));
                  qDebug("getProperty: invalid P_TYPE::LAYOUT_BREAK: <%s>", qPrintable(value));
                  }
                  break;
            case P_TYPE::VALUE_TYPE: {
                  if (value == "offset")
                        return QVariant(int(Note::ValueType::OFFSET_VAL));
                  else if (value == "user")
                        return QVariant(int(Note::ValueType::USER_VAL));
                  }
                  break;
            case P_TYPE::PLACEMENT: {
                  if (value == "above")
                        return QVariant(int(Placement::ABOVE));
                  else if (value == "below")
                        return QVariant(int(Placement::BELOW));
                  }
                  break;
            case P_TYPE::HPLACEMENT: {
                  if (value == "left")
                        return QVariant(int(HPlacement::LEFT));
                  else if (value == "center")
                        return QVariant(int(HPlacement::CENTER));
                  else if (value == "right")
                        return QVariant(int(HPlacement::RIGHT));
                  }
                  break;
            case P_TYPE::TEXT_PLACE: {
                  if (value == "auto")
                        return QVariant(int(PlaceText::AUTO));
                  else if (value == "above")
                        return QVariant(int(PlaceText::ABOVE));
                  else if (value == "below")
                        return QVariant(int(PlaceText::BELOW));
                  else if (value == "left")
                        return QVariant(int(PlaceText::LEFT));
                  }
                  break;
            case P_TYPE::BARLINE_TYPE: {
                  bool ok;
                  const int ct = value.toInt(&ok);
                  if (ok)
                        return QVariant(ct);
                  else {
                        BarLineType t = BarLine::barLineType(value);
                        return QVariant::fromValue(t);
                        }
                  }
                  break;
            case P_TYPE::BEAM_MODE:             // TODO
                  return QVariant(int(0));

            case P_TYPE::GROUPS:
                  // unsupported
                  return QVariant();
            case P_TYPE::SYMID:
                  return QVariant::fromValue(Sym::name2id(value));
            case P_TYPE::HEAD_SCHEME:
                  return QVariant::fromValue(NoteHead::name2scheme(value));
            case P_TYPE::HEAD_GROUP:
                  return QVariant::fromValue(NoteHead::name2group(value));
            case P_TYPE::HEAD_TYPE:
                  return QVariant::fromValue(NoteHead::name2type(value));
            case P_TYPE::POINT_MM:              // not supported
            case P_TYPE::TDURATION:
            case P_TYPE::SIZE_MM:
            case P_TYPE::INT_LIST:
                  return QVariant();
            case P_TYPE::SUB_STYLE:
                  return int(textStyleFromName(value));
            case P_TYPE::ALIGN: {
                  QStringList sl = value.split(',');
                  if (sl.size() != 2) {
                        qDebug("bad align text <%s>", qPrintable(value));
                        return QVariant();
                        }
                  Align align = Align::LEFT;
                  if (sl[0] == "center")
                        align = align | Align::HCENTER;
                  else if (sl[0] == "right")
                        align = align | Align::RIGHT;
                  else if (sl[0] == "left")
                        ;
                  else {
                        qDebug("bad align text <%s>", qPrintable(sl[0]));
                        return QVariant();
                        }
                  if (sl[1] == "center")
                        align = align | Align::VCENTER;
                  else if (sl[1] == "bottom")
                        align = align | Align::BOTTOM;
                  else if (sl[1] == "baseline")
                        align = align | Align::BASELINE;
                  else if (sl[1] == "top")
                        ;
                  else {
                        qDebug("bad align text <%s>", qPrintable(sl[1]));
                        return QVariant();
                        }
                  return int(align);
                  }
            case P_TYPE::CHANGE_METHOD:
                  return QVariant(int(ChangeMap::nameToChangeMethod(value)));
            case P_TYPE::ORIENTATION:
                  if (value == "vertical")
                        return QVariant(int(Orientation::VERTICAL));
                  else if (value == "horizontal")
                        return QVariant(int(Orientation::HORIZONTAL));
                  break;
            default:
                  break;
            }
      return QVariant();
      }

//---------------------------------------------------------
//    readProperty
//---------------------------------------------------------

QVariant readProperty(Pid id, XmlReader& e)
      {
      switch (propertyType(id)) {
            case P_TYPE::BOOL:
                  return QVariant(bool(e.readInt()));
            case P_TYPE::ZERO_INT:
            case P_TYPE::INT:
                  return QVariant(e.readInt());
            case P_TYPE::REAL:
            case P_TYPE::SPATIUM:
            case P_TYPE::SP_REAL:
            case P_TYPE::TEMPO:
                  return QVariant(e.readDouble());
            case P_TYPE::FRACTION:
                  return QVariant::fromValue(e.readFraction());
            case P_TYPE::COLOR:
                  return QVariant(e.readColor());
            case P_TYPE::POINT:
            case P_TYPE::POINT_SP:
            case P_TYPE::POINT_SP_MM:
                  return QVariant(e.readPoint());
            case P_TYPE::SCALE:
            case P_TYPE::SIZE:
                  return QVariant(e.readSize());
            case P_TYPE::FONT:
            case P_TYPE::STRING:
                  return QVariant(e.readElementText());
            case P_TYPE::GLISS_STYLE:
            case P_TYPE::ORNAMENT_STYLE:
            case P_TYPE::DIRECTION:
            case P_TYPE::DIRECTION_H:
            case P_TYPE::LAYOUT_BREAK:
            case P_TYPE::VALUE_TYPE:
            case P_TYPE::PLACEMENT:
            case P_TYPE::HPLACEMENT:
            case P_TYPE::TEXT_PLACE:
            case P_TYPE::BARLINE_TYPE:
            case P_TYPE::SYMID:
            case P_TYPE::HEAD_SCHEME:
            case P_TYPE::HEAD_GROUP:
            case P_TYPE::HEAD_TYPE:
            case P_TYPE::SUB_STYLE:
            case P_TYPE::ALIGN:
            case P_TYPE::ORIENTATION:
                  return propertyFromString(id, e.readElementText());

            case P_TYPE::BEAM_MODE:             // TODO
                  return QVariant(int(0));

            case P_TYPE::GROUPS:
                  {
                  Groups g;
                  g.read(e);
                  return QVariant::fromValue(g);
                  }
            case P_TYPE::POINT_MM:              // not supported
            case P_TYPE::TDURATION:
            case P_TYPE::SIZE_MM:
            case P_TYPE::INT_LIST:
                  return QVariant();
            default:
                  qFatal("unhandled PID type");
                  break;
            }
      return QVariant();
      }

//---------------------------------------------------------
//   propertyToString
//    Originally extracted from XmlWriter
//---------------------------------------------------------

QString propertyToString(Pid id, QVariant value, bool mscx)
      {
      if (value == QVariant())
            return QString();

      switch(id) {
            case Pid::SYSTEM_BRACKET: // system bracket type
                  return Bracket::bracketTypeName(BracketType(value.toInt()));
            case Pid::ACCIDENTAL_TYPE:
                  return Accidental::subtype2name(AccidentalType(value.toInt()));
            case Pid::OTTAVA_TYPE:
                  return Ottava::ottavaTypeName(OttavaType(value.toInt()));
            case Pid::TREMOLO_TYPE:
                  return Tremolo::type2name(TremoloType(value.toInt()));
            case Pid::TRILL_TYPE:
                  return Trill::type2name(Trill::Type(value.toInt()));
            case Pid::VIBRATO_TYPE:
                  return Vibrato::type2name(Vibrato::Type(value.toInt()));
            default:
                  break;
            }

      switch (propertyType(id)) {
            case P_TYPE::BOOL:
            case P_TYPE::INT:
            case P_TYPE::ZERO_INT:
                  return QString::number(value.toInt());
            case P_TYPE::REAL:
                  return QString::number(value.value<double>());
            case P_TYPE::SPATIUM:
                  return QString::number(value.value<Spatium>().val());
            case P_TYPE::DIRECTION:
                  return toString(value.value<Direction>());
            case P_TYPE::STRING:
            case P_TYPE::FRACTION:
                  return value.toString();
            case P_TYPE::KEYMODE:
                  switch (KeyMode(value.toInt())) {
                        case KeyMode::NONE:
                              return "none";
                        case KeyMode::MAJOR:
                              return "major";
                        case KeyMode::MINOR:
                              return "minor";
                        case KeyMode::DORIAN:
                              return "dorian";
                        case KeyMode::PHRYGIAN:
                              return "phrygian";
                        case KeyMode::LYDIAN:
                              return "lydian";
                        case KeyMode::MIXOLYDIAN:
                              return "mixolydian";
                        case KeyMode::AEOLIAN:
                              return "aeolian";
                        case KeyMode::IONIAN:
                              return "ionian";
                        case KeyMode::LOCRIAN:
                              return "locrian";
                        default:
                              return "unknown";
                        }
            case P_TYPE::ORNAMENT_STYLE:
                  switch (MScore::OrnamentStyle(value.toInt())) {
                        case MScore::OrnamentStyle::BAROQUE:
                              return "baroque";
                        case MScore::OrnamentStyle::DEFAULT:
                              return "default";
                        }
                  break;
            case P_TYPE::GLISS_STYLE:
                  switch (GlissandoStyle(value.toInt())) {
                        case GlissandoStyle::BLACK_KEYS:
                              return "blackkeys";
                        case GlissandoStyle::WHITE_KEYS:
                              return "whitekeys";
                        case GlissandoStyle::DIATONIC:
                              return "diatonic";
                        case GlissandoStyle::PORTAMENTO:
                              return "portamento";
                        case GlissandoStyle::CHROMATIC:
                              return "Chromatic";
                        }
                  break;
            case P_TYPE::DIRECTION_H:
                  switch (MScore::DirectionH(value.toInt())) {
                        case MScore::DirectionH::LEFT:
                              return "left";
                        case MScore::DirectionH::RIGHT:
                              return "right";
                        case MScore::DirectionH::AUTO:
                              return "auto";
                        }
                  break;
            case P_TYPE::LAYOUT_BREAK:
                  switch (LayoutBreak::Type(value.toInt())) {
                        case LayoutBreak::LINE:
                              return "line";
                        case LayoutBreak::PAGE:
                              return "page";
                        case LayoutBreak::SECTION:
                              return "section";
                        case LayoutBreak::NOBREAK:
                              return "nobreak";
                        }
                  break;
            case P_TYPE::VALUE_TYPE:
                  switch (Note::ValueType(value.toInt())) {
                        case Note::ValueType::OFFSET_VAL:
                              return "offset";
                        case Note::ValueType::USER_VAL:
                              return "user";
                        }
                  break;
            case P_TYPE::PLACEMENT:
                  switch (Placement(value.toInt())) {
                        case Placement::ABOVE:
                              return "above";
                        case Placement::BELOW:
                              return "below";
                        }
                  break;
            case P_TYPE::HPLACEMENT:
                  switch (HPlacement(value.toInt())) {
                        case HPlacement::LEFT:
                              return "left";
                        case HPlacement::CENTER:
                              return "center";
                        case HPlacement::RIGHT:
                              return "right";
                        }
                  break;
            case P_TYPE::TEXT_PLACE:
                  switch (PlaceText(value.toInt())) {
                        case PlaceText::AUTO:
                              return "auto";
                        case PlaceText::ABOVE:
                              return "above";
                        case PlaceText::BELOW:
                              return "below";
                        case PlaceText::LEFT:
                              return "left";
                        }
                  break;
            case P_TYPE::SYMID:
                  return Sym::id2name(SymId(value.toInt()));
            case P_TYPE::BARLINE_TYPE:
                  return BarLine::barLineTypeName(BarLineType(value.toInt()));
            case P_TYPE::HEAD_SCHEME:
                  return NoteHead::scheme2name(NoteHead::Scheme(value.toInt()));
            case P_TYPE::HEAD_GROUP:
                  return NoteHead::group2name(NoteHead::Group(value.toInt()));
            case P_TYPE::HEAD_TYPE:
                  return NoteHead::type2name(NoteHead::Type(value.toInt()));
            case P_TYPE::SUB_STYLE:
                  return textStyleName(Tid(value.toInt()));
            case P_TYPE::CHANGE_SPEED:
                  return Dynamic::speedToName(Dynamic::Speed(value.toInt()));
            case P_TYPE::CHANGE_METHOD:
                  return ChangeMap::changeMethodToName(ChangeMethod(value.toInt()));
            case P_TYPE::CLEF_TYPE:
                  return ClefInfo::tag(ClefType(value.toInt()));
            case P_TYPE::DYNAMIC_TYPE:
                  return Dynamic::dynamicTypeName(value.value<Dynamic::Type>());
            case P_TYPE::ALIGN: {
                  const Align a = Align(value.toInt());
                  const char* h;
                  if (a & Align::HCENTER)
                        h = "center";
                  else if (a & Align::RIGHT)
                        h = "right";
                  else
                        h = "left";
                  const char* v;
                  if (a & Align::BOTTOM)
                        v = "bottom";
                  else if (a & Align::VCENTER)
                        v = "center";
                  else if (a & Align::BASELINE)
                        v = "baseline";
                  else
                        v = "top";
                  return QString("%1,%2").arg(h, v);
                  }
            case P_TYPE::ORIENTATION: {
                  const Orientation o = Orientation(value.toInt());
                  if (o == Orientation::VERTICAL)
                        return "vertical";
                  else if (o == Orientation::HORIZONTAL)
                        return "horizontal";
                  break;
                  }
            case P_TYPE::POINT_MM:
                  qFatal("unknown: POINT_MM");
            case P_TYPE::SIZE_MM:
                  qFatal("unknown: SIZE_MM");
            case P_TYPE::TDURATION:
                  qFatal("unknown: TDURATION");
            case P_TYPE::BEAM_MODE:
                  qFatal("unknown: BEAM_MODE");
            case P_TYPE::TEMPO:
                  qFatal("unknown: TEMPO");
            case P_TYPE::GROUPS:
                  qFatal("unknown: GROUPS");
            case P_TYPE::INT_LIST:
                  qFatal("unknown: INT_LIST");

            default: {
                  switch(value.type()) {
                        case QVariant::Bool:
                        case QVariant::Char:
                        case QVariant::Int:
                        case QVariant::UInt:
                              return QString::number(value.toInt());
                        case QVariant::Double:
                              return QString::number(value.value<double>());
                        default:
                              break;
                        }
                  }
            }

      if (!mscx) {
            // String representation for properties that are written
            // to MSCX in other way (e.g. as XML tag properties).
            switch(value.type()) {
                  case QVariant::PointF: {
                        const QPointF p(value.value<QPointF>());
                        return QString("%1;%2").arg(QString::number(p.x()), QString::number(p.y()));
                        }
                  case QVariant::SizeF: {
                        const QSizeF s(value.value<QSizeF>());
                        return QString("%1x%2").arg(QString::number(s.width()), QString::number(s.height()));
                        }
                  // TODO: support QVariant::Rect and QVariant::QRectF?
                  default:
                        break;
                  }

            if (value.canConvert<QString>())
                  return value.toString();
            }

      return QString();
      }
}

