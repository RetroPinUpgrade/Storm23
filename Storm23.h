 
#define LAMP_BONUS_MULT_1                   11
#define LAMP_BONUS_MULT_2                   12
#define LAMP_BONUS_MULT_3                   13
#define LAMP_BONUS_MULT_4                   14
#define LAMP_BONUS_MULT_5                   15
#define LAMP_123_FOR_SPECIAL                16
#define LAMP_TARGET_1                       19
#define LAMP_TARGET_2                       20
#define LAMP_TARGET_3                       21
#define LAMP_TARGET_4                       22
#define LAMP_TARGET_5                       23
#define LAMP_TARGET_6                       24
#define LAMP_TARGET_7                       25
#define LAMP_TARGET_8                       26
#define LAMP_TARGET_9                       27
#define LAMP_TOP_DROP_SPECIAL               28
#define LAMP_MIDDLE_DROP_SPECIAL            29
#define LAMP_LEFT_DROP_SPECIAL              30
#define LAMP_TOP_EXTRA_BALL                 31
#define LAMP_MIDDLE_EXTRA_BALL              32
#define LAMP_LEFT_EXTRA_BALL                33
#define LAMP_TOP_DROP_BANK                  34
#define LAMP_MIDDLE_DROP_BANK               35
#define LAMP_LEFT_DROP_BANK                 36
#define LAMP_TOP_DROP_BANK_COMPLETE         37
#define LAMP_MIDDLE_DROP_BANK_COMPLETE      38
#define LAMP_LEFT_DROP_BANK_COMPLETE        39
#define LAMP_SHOOT_AGAIN                    40
#define LAMP_RIGHT_ADV_MULTIPLIER           41
#define LAMP_LEFT_ADV_MULTIPLIER            42
#define LAMP_SPINNER                        43
#define LAMP_RIGHT_INLANE_BOTTOM            44
#define LAMP_LEFT_INLANE_BOTTOM             45
#define LAMP_RIGHT_RETURN_TOP               46
#define LAMP_LEFT_RETURN_TOP                47
#define LAMP_HEAD_HIGH_SCORE                48
#define LAMP_ADV_BONUS_MULTIPLIER           49
#define LAMP_HEAD_GAME_OVER                 50
#define LAMP_HEAD_TILT                      51
#define LAMP_PLAYFIELD_1X                   52
#define LAMP_PLAYFIELD_2X                   53
#define LAMP_PLAYFIELD_3X                   54
#define LAMP_PLAYFIELD_4X                   55


#define SW_COIN_1                 0
#define SW_COIN_2                 1
#define SW_COIN_3                 2
#define SW_OUTHOLE                3
#define SW_SPINNER                4
#define SW_CREDIT_RESET           5
#define SW_PLUMB_TILT             6
#define SW_TILT                   6
#define SW_SLAM                   6
#define SW_TARGET_9               7
#define SW_RIGHT_INLANE_BOTTOM    8
#define SW_LEFT_INLANE_BOTTOM     9
#define SW_LOOP                   10
#define SW_OUTLANES_BOTTOM        11
#define SW_LEFT_SLING_BOTTOM      12
#define SW_LEFT_SLING_TOP         13
#define SW_RIGHT_SLING_BOTTOM     14
#define SW_RIGHT_SLING_TOP        15
#define SW_RIGHT_RETURN_LANE_TOP        16
#define SW_LEFT_RETURN_LANE_TOP         17
#define SW_DROP_2_3               18
#define SW_DROP_2_2               19
#define SW_DROP_2_1               20
#define SW_DROP_1_3               21
#define SW_DROP_1_2               22
#define SW_DROP_1_1               23
#define SW_TARGET_8               24
#define SW_TARGET_7               25
#define SW_TARGET_6               26
#define SW_TARGET_5               27
#define SW_TARGET_4               28
#define SW_TARGET_3               29
#define SW_TARGET_2               30
#define SW_TARGET_1               31
#define SW_TROUGH_3               32
#define SW_TROUGH_2               33
#define SW_TROUGH_1               34
#define SW_SAUCER_2               35
#define SW_SAUCER_1               36
#define SW_DROP_3_3               37
#define SW_DROP_3_2               38
#define SW_DROP_3_1               39


#define SOL_RIGHT_SLING_TOP       0
#define SOL_RIGHT_SLING           1
#define SOL_LEFT_SLING_TOP        2
#define SOL_LEFT_SLING            3
#define SOL_DROP_1_RESET          4
#define SOL_KNOCKER               5
#define SOL_OUTHOLE               6
#define SOL_BALL_SERVE            7
#define SOL_DROP_2_RESET          8
#define SOL_DROP_3_RESET          9
#define SOL_SAUCER_1              10
#define SOL_SAUCER_2              11


#define CONTSOL_BONUS_BOARD_TOGGLE    0x80

// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
#define NUM_SWITCHES_WITH_TRIGGERS         4

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 4

// Define automatic solenoid triggers (switch, solenoid, number of 1/120ths of a second to fire)
struct PlayfieldAndCabinetSwitch SolenoidAssociatedSwitches[] = {
  { SW_LEFT_SLING_TOP, SOL_LEFT_SLING_TOP, 4 },
  { SW_LEFT_SLING_BOTTOM, SOL_LEFT_SLING, 4 },
  { SW_RIGHT_SLING_TOP, SOL_RIGHT_SLING_TOP, 4 },
  { SW_RIGHT_SLING_BOTTOM, SOL_RIGHT_SLING, 4 },
};
