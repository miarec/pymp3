#pragma once

enum pymp3_mpeg_layer {
  LAYER_I   = 1,			/* Layer I */
  LAYER_II  = 2,			/* Layer II */
  LAYER_III = 3			/* Layer III */
};

enum pymp3_mpeg_mode {
  MODE_SINGLE_CHANNEL = 0,		/* single channel */
  MODE_DUAL_CHANNEL	  = 1,		/* dual channel */
  MODE_JOINT_STEREO	  = 2,		/* joint (MS/intensity) stereo */
  MODE_STEREO	  = 3		/* normal LR stereo */
};