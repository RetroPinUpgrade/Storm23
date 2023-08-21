/**************************************************************************
    Storm23 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>. 
*/

#include "RPU_Config.h"
#include "RPU.h"
#include "DropTargets.h"
#include "Storm23.h"
#include "SelfTestAndAudit.h"
#include "AudioHandler.h"
#include "LampAnimations.h"
#include <EEPROM.h>

//  "Music by Karl Casey @ White Bat Audio"

// Bugs & stuff
//    credit display test needs unmasking
//    better callouts/animations for multiball
//    extra ball? special?
//    earlier targets should be worth something in jackpot+
//    lame kick from saucer in spinner multi immediately went to sad music
//    multi stil qualified after someone kicked it?
//    multi balls should be unqualified when storm (or others?) are kicked
//    outlanes - no sound?
//    Should we be locking a ball if multiball is running!?
//        if (!multiballRunning) QueueNotification(SOUND_EFFECT_VP_BALL_1_LOCKED, 10);
//        LockBall(0);
//    multi still qualified after locks have been ejected?
//    ball missing with 4 player reverts to 1 player game?
//

#define STORM_MAJOR_VERSION  2023
#define STORM_MINOR_VERSION  2
#define DEBUG_MESSAGES  0


/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110
#define MACHINE_STATE_DIAGNOSTICS     120

#define MACHINE_STATE_ADJUST_FREEPLAY             (MACHINE_STATE_TEST_DONE-1)
#define MACHINE_STATE_ADJUST_BALL_SAVE            (MACHINE_STATE_TEST_DONE-2)
#define MACHINE_STATE_ADJUST_SOUND_SELECTOR       (MACHINE_STATE_TEST_DONE-3)
#define MACHINE_STATE_ADJUST_MUSIC_VOLUME         (MACHINE_STATE_TEST_DONE-4)
#define MACHINE_STATE_ADJUST_SFX_VOLUME           (MACHINE_STATE_TEST_DONE-5)
#define MACHINE_STATE_ADJUST_CALLOUTS_VOLUME      (MACHINE_STATE_TEST_DONE-6)
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING   (MACHINE_STATE_TEST_DONE-7)
#define MACHINE_STATE_ADJUST_TILT_WARNING         (MACHINE_STATE_TEST_DONE-8)
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE       (MACHINE_STATE_TEST_DONE-9)
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE       (MACHINE_STATE_TEST_DONE-10)
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES     (MACHINE_STATE_TEST_DONE-11)
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD     (MACHINE_STATE_TEST_DONE-12)
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD        (MACHINE_STATE_TEST_DONE-13)
#define MACHINE_STATE_ADJUST_RESET_HOLD_TIME      (MACHINE_STATE_TEST_DONE-14)
#define MACHINE_STATE_ADJUST_SOUND_TRACK          (MACHINE_STATE_TEST_DONE-15)
#define MACHINE_STATE_ADJUST_SAUCER_EJECT         (MACHINE_STATE_TEST_DONE-16)
#define MACHINE_STATE_ADJUST_MULTIBALL_BALL_SAVE  (MACHINE_STATE_TEST_DONE-17)
#define MACHINE_STATE_ADJUST_SPINS_UNTIL_MB       (MACHINE_STATE_TEST_DONE-18)
#define MACHINE_STATE_ADJUST_DONE                 (MACHINE_STATE_TEST_DONE-19)

byte SelfTestStateToCalloutMap[] = {
  136, 137, 135, 134, 133, 140, 141, 142, 139, 143, 144, 145, 146, 147, 148, 149, 138, 150, 151, 152, // <- SelfTestAndAudit modes
  // Starting Storm23 specific modes
  153, 154, 155, 156, 157, 158, // Freeplay through Callouts volume
  159, 160, 161, 162, 163, 164, 165, // through special award
  171, 172, 173, 174, 175, // Allow reset, sound track, saucer eject
  0
};



#define GAME_MODE_SKILL_SHOT                        1
#define GAME_MODE_UNSTRUCTURED_PLAY                 2
#define GAME_MODE_LOCK_SEQUENCE_1                   3
#define GAME_MODE_LOCK_SEQUENCE_2                   4
#define GAME_MODE_STORM_ADD_A_BALL                  5


#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_SOUND_SELECTOR_BYTE      102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_SFX_VOLUME_BYTE          108
#define EEPROM_MUSIC_VOLUME_BYTE        109
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_CALLOUTS_VOLUME_BYTE     111
#define EEPROM_GOALS_UNTIL_WIZ_BYTE     112
#define EEPROM_IDLE_MODE_BYTE           113
#define EEPROM_WIZ_TIME_BYTE            114
#define EEPROM_SPINNER_ACCELERATOR_BYTE 115
#define EEPROM_COMBOS_GOAL_BYTE         116
#define EEPROM_RESET_HOLD_TIME_BYTE     117
#define EEPROM_SOUND_TRACK_BYTE         118
#define EEPROM_SAUCER_EJECT_BYTE        119
#define EEPROM_MB_BALL_SAVE_BYTE        120
#define EEPROM_SPINS_UNTIL_MB_BYTE      121
#define EEPROM_EXTRA_BALL_SCORE_UL      140
#define EEPROM_SPECIAL_SCORE_UL         144


#define SOUND_EFFECT_NONE                               0
#define SOUND_EFFECT_BONUS_ADD                          1
#define SOUND_EFFECT_BONUS_COLLECT                      2
#define SOUND_EFFECT_TILT_WARNING                       3
#define SOUND_EFFECT_TILT                               4
#define SOUND_EFFECT_SCORE_TICK                         5
#define SOUND_EFFECT_DROP_TARGET_HIT                    6
#define SOUND_EFFECT_DROP_TARGET_NO_POINTS              7
#define SOUND_EFFECT_DROP_TARGET_VALUE_HIT              8
#define SOUND_EFFECT_DROP_TARGET_RESET                  9
#define SOUND_EFFECT_DROP_TARGET_STAGE_COMPLETE         10
#define SOUND_EFFECT_DROP_TARGET_MODE_COMPLETE          11
#define SOUND_EFFECT_MATCH_SPIN                         12
#define SOUND_EFFECT_SLING_SHOT                         13
#define SOUND_EFFECT_SPINNER                            14
#define SOUND_EFFECT_ALTERNATION_ADVANCE                15
#define SOUND_EFFECT_SPINNER_LIT                        16
#define SOUND_EFFECT_LOOP                               17
#define SOUND_EFFECT_LOOP_LIT                           18
#define SOUND_EFFECT_INLANE                             19
#define SOUND_EFFECT_INLANE_LIT                         20
#define SOUND_EFFECT_STANDUP                            21
#define SOUND_EFFECT_STANDUP_LIT                        22
#define SOUND_EFFECT_SAUCER_REJECTED                    23
#define SOUND_EFFECT_DROP_TARGET_STORM_HIT              24
#define SOUND_EFFECT_DROP_TARGET_STORM_CLEARED          25
#define SOUND_EFFECT_ADD_CREDIT                         26
#define SOUND_EFFECT_SPINNER_MAX                        27
#define SOUND_EFFECT_LIGHTNING_1                        28
#define SOUND_EFFECT_LIGHTNING_2                        29
#define SOUND_EFFECT_LIGHTNING_3                        30
#define SOUND_EFFECT_LIGHTNING_4                        31
#define SOUND_EFFECT_LIGHTNING_5                        32
#define SOUND_EFFECT_LIGHTNING_6                        33
#define SOUND_EFFECT_LIGHTNING_7                        34
#define SOUND_EFFECT_LIGHTNING_8                        35
#define SOUND_EFFECT_LIGHTNING_9                        36
#define SOUND_EFFECT_LIGHTNING_10                       37
#define SOUND_EFFECT_LIGHTNING_11                       38
#define SOUND_EFFECT_LIGHTNING_12                       39
#define SOUND_EFFECT_SAUCER_HOLD                        40

#define SOUND_EFFECT_COIN_DROP_1                        100
#define SOUND_EFFECT_COIN_DROP_2                        101
#define SOUND_EFFECT_COIN_DROP_3                        102
#define SOUND_EFFECT_MACHINE_START                      120

#define SOUND_EFFECT_SELF_TEST_CPC_START                180
#define SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START      190
#define SOUND_EFFECT_SELF_TEST_CRB_OPTIONS_START        210
#define SOUND_EFFECT_SELF_TEST_MBBS_WARNING             220


// Game play status callouts
#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START                 300
#define SOUND_EFFECT_VP_BALL_MISSING                              300
#define SOUND_EFFECT_VP_PLAYER_ONE_UP                             301
#define SOUND_EFFECT_VP_PLAYER_TWO_UP                             302
#define SOUND_EFFECT_VP_PLAYER_THREE_UP                           303
#define SOUND_EFFECT_VP_PLAYER_FOUR_UP                            304
#define SOUND_EFFECT_VP_JACKPOT                                   305
#define SOUND_EFFECT_VP_DOUBLE_JACKPOT                            306
#define SOUND_EFFECT_VP_SUPER_JACKPOT                             307
#define SOUND_EFFECT_VP_MEGA_JACKPOT                              308
#define SOUND_EFFECT_VP_TIMERS_DOUBLED                            309

#define SOUND_EFFECT_VP_ADD_PLAYER_1                              310
#define SOUND_EFFECT_VP_ADD_PLAYER_2                              (SOUND_EFFECT_VP_ADD_PLAYER_1+1)
#define SOUND_EFFECT_VP_ADD_PLAYER_3                              (SOUND_EFFECT_VP_ADD_PLAYER_1+2)
#define SOUND_EFFECT_VP_ADD_PLAYER_4                              (SOUND_EFFECT_VP_ADD_PLAYER_1+3)

#define SOUND_EFFECT_VP_SHOOT_AGAIN                               314
#define SOUND_EFFECT_VP_EXTRA_BALL                                315
#define SOUND_EFFECT_VP_BALL_LOCKED                               316
#define SOUND_EFFECT_VP_SPINNER_FRENZY                            317
#define SOUND_EFFECT_VP_DROP_TARGET_FRENZY                        318
#define SOUND_EFFECT_VP_COMBO_1                                   319
#define SOUND_EFFECT_VP_COMBO_2                                   320
#define SOUND_EFFECT_VP_COMBO_3                                   321
#define SOUND_EFFECT_VP_COMBO_4                                   322
#define SOUND_EFFECT_VP_COMBO_5                                   323
#define SOUND_EFFECT_VP_COMBO_6                                   324
#define SOUND_EFFECT_VP_COMBO_7                                   325
#define SOUND_EFFECT_VP_COMBO_8                                   326
#define SOUND_EFFECT_VP_BALL_1_LOCKED                             327
#define SOUND_EFFECT_VP_BALL_2_LOCKED                             328
#define SOUND_EFFECT_VP_LOCK_1_READY                              329
#define SOUND_EFFECT_VP_LOCK_2_READY                              330
#define SOUND_EFFECT_VP_SKILL_SHOT                                331
#define SOUND_EFFECT_VP_LOOP_MULTIBALL_START                      332
#define SOUND_EFFECT_VP_SPINNER_MULTIBALL_START                   333
#define SOUND_EFFECT_VP_STORM_MULTIBALL_START                     334
#define SOUND_EFFECT_VP_BONUS_X_HELD                              335
#define SOUND_EFFECT_VP_BONUS_HELD                                336
#define SOUND_EFFECT_VP_CAPTURE_THE_VALKYRIE                      337
#define SOUND_EFFECT_VP_ODIN_PLEASED                              338
#define SOUND_EFFECT_VP_RETURN_TO_VALHALLA                        339
#define SOUND_EFFECT_VP_END_DROP_TARGET_FRENZY                    340
#define SOUND_EFFECT_VP_END_SPINNER_FRENZY                        341
#define SOUND_EFFECT_VP_JAILBREAK_MULTIBALL                       342


#define SOUND_EFFECT_RALLY_SONG_1                 500
#define SOUND_EFFECT_RALLY_SONG_2                 501
#define SOUND_EFFECT_RALLY_SONG_3                 502
#define SOUND_EFFECT_BACKGROUND_SONG_1            525
#define SOUND_EFFECT_BACKGROUND_SONG_2            526
#define SOUND_EFFECT_BACKGROUND_SONG_3            527
#define SOUND_EFFECT_BACKGROUND_AFTER_MULTI       530
#define SOUND_EFFECT_BACKGROUND_STORM_READY       550
#define SOUND_EFFECT_BACKGROUND_MULTIBALL         575


#define SOUND_EFFECT_DIAG_START                   1900
#define SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON     1900
#define SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON      1901
#define SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF     1902
#define SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE  1903
#define SOUND_EFFECT_DIAG_STARTING_NEW_CODE       1904
#define SOUND_EFFECT_DIAG_ORIGINAL_CPU_DETECTED   1905
#define SOUND_EFFECT_DIAG_ORIGINAL_CPU_RUNNING    1906
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_U10         1907
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_U11         1908
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_1           1909
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_2           1910
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_3           1911
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_4           1912
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_5           1913
#define SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS    1914


#define MAX_DISPLAY_BONUS               99
#define TILT_WARNING_DEBOUNCE_TIME      1000

/*********************************************************************

    Machine state and options

*********************************************************************/
byte Credits = 0;
byte SoundSelector = 6;
byte BallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte ScoreAwardReplay = 0;
byte MusicVolume = 6;
byte SoundEffectsVolume = 8;
byte CalloutsVolume = 10;
byte ChuteCoinsInProgress[3];
byte TotalBallsLoaded = 3;
byte TimeRequiredToResetGame = 1;
byte NumberOfBallsInPlay = 0;
byte NumberOfBallsLocked = 0;
byte LampType = 0;
boolean FreePlayMode = false;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long HighScore = 0;
unsigned long AwardScores[3];
unsigned long CreditResetPressStarted = 0;
unsigned long Saucer1EjectTime = 0;
unsigned long Saucer2EjectTime = 0;

AudioHandler Audio;



/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus[4];
byte BonusX[4];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte CurrentAchievements[4];

boolean SamePlayerShootsAgain = false;
boolean ExtraBallCollected = false;
boolean SpecialCollected = false;
boolean TimersPaused = true;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown;
unsigned long PlayfieldMultiplier;
unsigned long LastTimeThroughLoop;
unsigned long LastSwitchHitTime;
unsigned long BallSaveEndTime;

#define BALL_SAVE_GRACE_PERIOD        2000
#define BALL_SERVE_SOLENOID_TIME      60
#define OUTHOLE_KICKER_SOLENOID_TIME  24
#define KNOCKER_SOLENOID_TIME         16
#define SAUCER_KICKOUT_TIME           0

/*********************************************************************

    Game Specific State Variables

*********************************************************************/
byte TotalSpins[4];
byte SpinnerMaxGoal[4];
byte IdleMode;
byte SkillShotTarget;
byte DropTargetMode[4];
byte DropTargetStage[4];
byte AlternationMode[4];
byte ReturnLaneStatus[4];
byte LoopSpinnerAlternation;
byte StandupLevel[4];
byte LoopMultiballCounter;
byte LoopMulitballGoal[4];
byte BonusXIncreaseAvailable[4];
byte HoldoverAwards[4];
byte SoundTrackNum = 1;
byte SaucerEjectStrength = 1;
byte MultiballBallSave = 0;
byte SpinsUntilMultiball = 75;

#define HOLDOVER_BONUS_X            0x01
#define HOLDOVER_BONUS              0x02
#define HOLDOVER_TIMER_MULTIPLIER   0x04

#define NUM_BALL_SEARCH_SOLENOIDS   4
byte BallSearchSolenoidToTry;
byte BallSearchSols[NUM_BALL_SEARCH_SOLENOIDS] = {SOL_LEFT_SLING_TOP, SOL_RIGHT_SLING_TOP, SOL_LEFT_SLING, SOL_RIGHT_SLING};
#define BALL_SEARCH_POP_INDEX 0
#define BALL_SEARCH_LEFT_SLING_INDEX  1
#define BALL_SEARCH_RIGHT_SLING_INDEX 2

// Machine locks is a curated variable that keeps debounced
// knowledge of how many balls are trapped in different mechanisms.
// It's initialized the first time that attract mode is run,
// and then it's maintained by "UpdateLockStatus", which only 
// registers change when a state is changed for a certain
// amount of time.
byte MachineLocks; 
byte PlayerLockStatus[4];

#define LOCK_1_ENGAGED        0x10
#define LOCK_2_ENGAGED        0x20
#define LOCKS_ENGAGED_MASK    0x30
#define LOCK_1_AVAILABLE      0x01
#define LOCK_2_AVAILABLE      0x02
#define LOCKS_AVAILABLE_MASK  0x03

unsigned long FeaturesCompleted[4];

#define FEATURE_DROP_TARGET_FRENZY    0x01
#define FEATURE_SPINNER_FRENZY        0x02
#define FEATURE_LOOP_MULTIBALL        0x04
#define FEATURE_SPINNER_MULTIBALL     0x08
#define FEATURE_STORM_MULTIBALL       0x10
#define FEATURE_ALL_COMBOS            0x20
#define FEATURE_JACKPOT_AWARDED       0x40


boolean IdleModeEnabled = true;
boolean OutlaneSpecialLit[4];
boolean DropTargetHurryLamp[3];
boolean LoopMultiballRunning;
boolean LoopMultiballJackpotReady;
boolean LoopMultiballDoubleJackpotReady;
boolean SpinnerMultiballRunning;
boolean SpinnerMultiballJackpotReady;
boolean SpinnerMultiballDoubleJackpotReady;
boolean StormMultiballReady[4];
boolean StormMultiballRunning;
boolean StormMultiballMegaJackpotReady;
boolean JailBreakMultiballRunning;
boolean JailBreakJackpotReady;

unsigned int StandupHits[4];
#define STANDUP_1             0x0001
#define STANDUP_2             0x0002
#define STANDUP_3             0x0004
#define STANDUP_4             0x0008
#define STANDUP_5             0x0010
#define STANDUP_6             0x0020
#define STANDUP_7             0x0040
#define STANDUP_8             0x0080
#define STANDUP_9             0x0100
#define STANDUPS_FOR_LOCK_1   0x007C
#define STANDUPS_FOR_LOCK_2   0x0183
#define STANDUPS_ALL          0x01FF

unsigned long LoopMultiballCountChangedTime;
unsigned long BonusXAnimationStart;
unsigned long LastSpinnerHit;

unsigned long PlayfieldMultiplierExpiration;
unsigned long BallSearchNextSolenoidTime;
unsigned long BallSearchSolenoidFireTime[NUM_BALL_SEARCH_SOLENOIDS];
unsigned long TicksCountedTowardsStatus;
unsigned long BonusChangedTime;
unsigned long BallRampKicked;
unsigned long DropTargetResetTime[3];
unsigned long DropTargetHurryTime[3];
unsigned long DropTargetFrenzyEndTime;
unsigned long SpinnerFrenzyEndTime;
unsigned long LoopSpinnerLastAlternationTime;
unsigned long SpinnerLitEndTime;
unsigned long StandupFlashEndTime[9];
unsigned long Saucer1ScheduledEject;
unsigned long Saucer2ScheduledEject;
unsigned long StormLampsLastChange;
unsigned long FrenzyDropLastHit;
unsigned long LastTurnaroundHit;
unsigned long LastSaucer1Hit;
unsigned long LastSaucer2Hit;
unsigned long PlayScoreAnimationTick = 2500;

// Combo tracking variables
unsigned long LastLeftInlane;
unsigned long LastRightInlane;
unsigned long CombosAchieved[4];

#define SPINNER_FRENZY_TIME             25000

#define COMBO_LEFT_TO_2BANK             0x01
#define COMBO_RIGHT_TO_2BANK            0x02
#define COMBO_LEFT_TO_LOOP              0x04
#define COMBO_RIGHT_TO_LOOP             0x08
#define COMBO_LEFT_TO_SPINNER           0x10
#define COMBO_RIGHT_TO_UPPER            0x20
#define COMBO_LEFT_TO_RIGHT             0x40
#define COMBO_RIGHT_TO_LEFT             0x80

#define DROP_TARGETS_IN_ORDER           0
#define DROP_TARGETS_FRENZY             1
#define DROP_TARGETS_JACKPOT            2
#define DROP_TARGETS_DOUBLE_JACKPOT     3
#define DROP_TARGETS_SUPER_JACKPOT      4
#define DROP_TARGETS_MEGA_JACKPOT       5
#define DROP_TARGET_FRENZY_TIME         30000
DropTargetBank DropTargets1(3, 1, DROP_TARGET_TYPE_STRN_1, 8);
DropTargetBank DropTargets2(3, 1, DROP_TARGET_TYPE_STRN_1, 8);
DropTargetBank DropTargets3(3, 1, DROP_TARGET_TYPE_STRN_1, 8);

#define IDLE_MODE_NONE                  0
#define IDLE_MODE_BALL_SEARCH           9

#define COMBO_EXPIRATION_TIME     2000
#define SKILL_SHOT_AWARD          25000
#define SUPER_SKILL_SHOT_AWARD    50000


#define DROP_TARGET_JACKPOT_VALUE             50000
#define LOOP_AND_SPINNER_JACKPOT_VALUE        10000
#define STORM_MULTIBALL_JACKPOT_VALUE         20000
#define LOOP_MULTIBALL_JACKPOT_VALUE          10000
#define SPINNER_MULTIBALL_JACKPOT_VALUE       37500


/******************************************************
 * 
 * Adjustments Serialization
 * 
 */


void ReadStoredParameters() {
   for (byte count=0; count<3; count++) {
    ChuteCoinsInProgress[count] = 0;
  }
 
  HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;

  SoundSelector = ReadSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
  if (SoundSelector > 9) SoundSelector = 6;

  MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
  if (MusicVolume>10) MusicVolume = 10;

  SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10);
  if (SoundEffectsVolume>10) SoundEffectsVolume = 10;

  CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
  if (CalloutsVolume>10) CalloutsVolume = 10;

  Audio.SetMusicVolume(MusicVolume);
  Audio.SetSoundFXVolume(SoundEffectsVolume);
  Audio.SetNotificationsVolume(CalloutsVolume);

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;

  TimeRequiredToResetGame = ReadSetting(EEPROM_RESET_HOLD_TIME_BYTE, 3);
  if (TimeRequiredToResetGame>3 && TimeRequiredToResetGame<99) {
    TimeRequiredToResetGame = 99;
  }

  SoundTrackNum = ReadSetting(EEPROM_SOUND_TRACK_BYTE, 1);
  if (SoundTrackNum==0 || SoundTrackNum>2) SoundTrackNum = 1;

  SaucerEjectStrength = ReadSetting(EEPROM_SAUCER_EJECT_BYTE, 1);
  if (SaucerEjectStrength==0) SaucerEjectStrength = 1;
  else if (SaucerEjectStrength>3) SaucerEjectStrength = 3;

  MultiballBallSave = ReadSetting(EEPROM_MB_BALL_SAVE_BYTE, 0);
  if (MultiballBallSave > 1) MultiballBallSave = 0;

  SpinsUntilMultiball = ReadSetting(EEPROM_SPINS_UNTIL_MB_BYTE, 75);
  if ((SpinsUntilMultiball%25)!=0 || SpinsUntilMultiball>125) SpinsUntilMultiball = 75;

  ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_UL);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_UL);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);

}


void MoveBallFromOutholeToRamp(boolean sawSwitch = false) {
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE) || sawSwitch) {
    if (CurrentTime==0 || CurrentTime>(BallRampKicked+1500)) {
      RPU_PushToSolenoidStack(SOL_OUTHOLE, OUTHOLE_KICKER_SOLENOID_TIME, true); 
      if (CurrentTime) BallRampKicked = CurrentTime;
      else BallRampKicked = millis();
    }
  }
  
}


void QueueDIAGNotification(unsigned short notificationNum) {
  // This is optional, but the machine can play an audio message at boot
  // time to indicate any errors and whether it's going to boot to original
  // or new code.
  Audio.QueuePrioritizedNotification(notificationNum, 0, 10, CurrentTime);
  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Diag = %d\n", notificationNum);
    Serial.write(buf);    
  }
}


#ifdef RPU_OS_USE_SB300
void InitSB300Registers() {
  RPU_PlaySB300SquareWave(1, 0x00); // Write 0x00 to CR2 (Timer 2 off, continuous mode, 16-bit, C2 clock, CR3 set)
  RPU_PlaySB300SquareWave(0, 0x00); // Write 0x00 to CR3 (Timer 3 off, continuous mode, 16-bit, C3 clock, not prescaled)
  RPU_PlaySB300SquareWave(1, 0x01); // Write 0x00 to CR2 (Timer 2 off, continuous mode, 16-bit, C2 clock, CR1 set)
  RPU_PlaySB300SquareWave(0, 0x00); // Write 0x00 to CR1 (Timer 1 off, continuous mode, 16-bit, C1 clock, timers allowed)
}


void PlaySB300StartupBeep() {
  RPU_PlaySB300SquareWave(1, 0x92); // Write 0x92 to CR2 (Timer 2 on, continuous mode, 16-bit, E clock, CR3 set)
  RPU_PlaySB300SquareWave(0, 0x92); // Write 0x92 to CR3 (Timer 3 on, continuous mode, 16-bit, E clock, not prescaled)
  RPU_PlaySB300SquareWave(4, 0x02); // Set Timer 2 to 0x0200
  RPU_PlaySB300SquareWave(5, 0x00); 
  RPU_PlaySB300SquareWave(6, 0x80); // Set Timer 3 to 0x8000
  RPU_PlaySB300SquareWave(7, 0x00);
  RPU_PlaySB300Analog(0, 0x02);
}
#endif


void setup() {

  if (DEBUG_MESSAGES) {
    // If debug is on, set up the Serial port for communication
    Serial.begin(115200);
    Serial.write("Starting\n");
  }

  // Set up the Audio handler in order to play boot messages
  CurrentTime = millis();
  if (DEBUG_MESSAGES) Serial.write("Staring Audio\n");
  Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
  Audio.StopAllAudio();

  // Tell the OS about game-specific lights and switches
  RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, SolenoidAssociatedSwitches);  

  // Set up the chips and interrupts
  unsigned long initResult = 0;
  if (DEBUG_MESSAGES) Serial.write("Initializing MPU\n");
  if (RPU_OS_HARDWARE_REV<=3) {
    initResult = RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED);
  } else {
    initResult = RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN | RPU_CMD_PERFORM_MPU_TEST | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED, SW_CREDIT_RESET);
  }
  
  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Return from init = 0x%04lX\n", initResult);
    Serial.write(buf);
    if (initResult&RPU_RET_6800_DETECTED) Serial.write("Detected 6800 clock\n");
    else if (initResult&RPU_RET_6802_OR_8_DETECTED) Serial.write("Detected 6802/8 clock\n");
    Serial.write("Back from init\n");
  }

  if (initResult & RPU_RET_SELECTOR_SWITCH_ON) QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON);
  else QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF);
  if (initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) QueueDIAGNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON);

  if (initResult & RPU_RET_DIAGNOSTIC_REQUESTED) {
    QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS);
    // Run diagnostics here:    
  }

  if (initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) {
    delay(100);
    if (RPU_OS_HARDWARE_REV<=3) delay(2000);
    QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE);
    delay(100);
    while (Audio.Update(millis()));
    // Arduino should hang if original code is running
    while (1);
  }
  if (RPU_OS_HARDWARE_REV>3) QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();
  RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  CurrentScores[0] = STORM_MAJOR_VERSION;
  CurrentScores[1] = STORM_MINOR_VERSION;
  CurrentScores[2] = RPU_OS_MAJOR_VERSION;
  CurrentScores[3] = RPU_OS_MINOR_VERSION;

  CurrentAchievements[0] = 0;
  CurrentAchievements[1] = 0;
  CurrentAchievements[2] = 0;
  CurrentAchievements[3] = 0;

  DropTargets1.DefineSwitch(0, SW_DROP_1_1);
  DropTargets1.DefineSwitch(1, SW_DROP_1_2);
  DropTargets1.DefineSwitch(2, SW_DROP_1_3);
  DropTargets1.DefineResetSolenoid(0, SOL_DROP_1_RESET);

  DropTargets2.DefineSwitch(0, SW_DROP_2_1);
  DropTargets2.DefineSwitch(1, SW_DROP_2_2);
  DropTargets2.DefineSwitch(2, SW_DROP_2_3);
  DropTargets2.DefineResetSolenoid(0, SOL_DROP_2_RESET);

  DropTargets3.DefineSwitch(0, SW_DROP_3_1);
  DropTargets3.DefineSwitch(1, SW_DROP_3_2);
  DropTargets3.DefineSwitch(2, SW_DROP_3_3);
  DropTargets3.DefineResetSolenoid(0, SOL_DROP_3_RESET);

  Audio.SetMusicDuckingGain(25);
#ifdef RPU_OS_USE_SB300
  InitSB300Registers();
  PlaySB300StartupBeep();
#endif
}

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}


// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (RPU_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}


////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////

void ShowBonusXLamps() {
  byte curBonusX = BonusX[CurrentPlayer];

  RPU_SetLampState(LAMP_BONUS_MULT_1, curBonusX==1 || curBonusX==6, 0, BonusXAnimationStart ? 100 : 0);
  RPU_SetLampState(LAMP_BONUS_MULT_2, curBonusX==2 || curBonusX==7, 0, BonusXAnimationStart ? 100 : 0);
  RPU_SetLampState(LAMP_BONUS_MULT_3, curBonusX==3 || curBonusX==8, 0, BonusXAnimationStart ? 100 : 0);
  RPU_SetLampState(LAMP_BONUS_MULT_4, curBonusX==4 || curBonusX==9, 0, BonusXAnimationStart ? 100 : 0);
  RPU_SetLampState(LAMP_BONUS_MULT_5, curBonusX>4, 0, BonusXAnimationStart ? 100 : (curBonusX==10?500:0));
}

void ShowPlayfieldMultiplierLamps() {
  boolean aboutToExpire = false;
  if (PlayfieldMultiplierExpiration && (CurrentTime+5000)>PlayfieldMultiplierExpiration) aboutToExpire = true;

  RPU_SetLampState(LAMP_PLAYFIELD_1X, PlayfieldMultiplier==1 || PlayfieldMultiplier==5, 0, aboutToExpire ? 100 : 0);
  RPU_SetLampState(LAMP_PLAYFIELD_2X, PlayfieldMultiplier==2 || PlayfieldMultiplier==6, 0, aboutToExpire ? 100 : 0);
  RPU_SetLampState(LAMP_PLAYFIELD_3X, PlayfieldMultiplier==3 || PlayfieldMultiplier==7, 0, aboutToExpire ? 100 : 0);
  RPU_SetLampState(LAMP_PLAYFIELD_4X, PlayfieldMultiplier>=4, 0, aboutToExpire ? 100 : 0);
  
}


// Upper left standups are 1 & 2
// Upper right (5) standups are 3, 4, 5, 6, 7
// lower right standups are 8 & 9

void ShowStandupLamps() {

  if (GameMode==GAME_MODE_SKILL_SHOT) {
    byte targetNum = ((CurrentTime-GameModeStartTime)/(650 - ((unsigned long)CurrentBallInPlay)*100))%8;
    if (targetNum>4) targetNum = 8 - targetNum;
    SkillShotTarget = targetNum + 2;

    byte targetBit = STANDUP_3;
    for (byte count=0; count<5; count++) {
      RPU_SetLampState(LAMP_TARGET_3+count, count==targetNum);
      targetBit *= 2;
    }
    RPU_SetLampState(LAMP_TARGET_1, 0);
    RPU_SetLampState(LAMP_TARGET_2, 0);
    RPU_SetLampState(LAMP_TARGET_8, 0);
    RPU_SetLampState(LAMP_TARGET_9, 0);
  } else if (StormMultiballReady[CurrentPlayer]) {
    unsigned int targetBit = STANDUP_1;
    for (byte count=0; count<9; count++) {
      RPU_SetLampState(LAMP_TARGET_1+count, (targetBit & StandupHits[CurrentPlayer])?true:false);
      targetBit *= 2;
    }
  } else {
    unsigned int targetBit = STANDUP_1;
    for (byte count=0; count<9; count++) {
      RPU_SetLampState(LAMP_TARGET_1+count, (targetBit & StandupHits[CurrentPlayer])||StandupFlashEndTime[count], 0, StandupFlashEndTime[count]?100:0);
      targetBit *= 2;
    }
  }

}


void ShowLaneAndRolloverLamps() {
  if (GameMode==GAME_MODE_SKILL_SHOT) {
    RPU_SetLampState(LAMP_LEFT_RETURN_TOP, 0);
    RPU_SetLampState(LAMP_RIGHT_RETURN_TOP, 0);
    RPU_SetLampState(LAMP_LEFT_INLANE_BOTTOM, 0);
    RPU_SetLampState(LAMP_RIGHT_INLANE_BOTTOM, 0);
  } else if (LastLeftInlane) {
    RPU_SetLampState(LAMP_LEFT_RETURN_TOP, 0);
    RPU_SetLampState(LAMP_RIGHT_RETURN_TOP, 0);
    RPU_SetLampState(LAMP_LEFT_INLANE_BOTTOM, 0);
    RPU_SetLampState(LAMP_RIGHT_INLANE_BOTTOM, (CombosAchieved[CurrentPlayer]&COMBO_LEFT_TO_RIGHT)==0, 0, 100);
  } else if (LastRightInlane) {
    RPU_SetLampState(LAMP_LEFT_RETURN_TOP, 0);
    RPU_SetLampState(LAMP_RIGHT_RETURN_TOP, 0);
    RPU_SetLampState(LAMP_LEFT_INLANE_BOTTOM, (CombosAchieved[CurrentPlayer]&COMBO_RIGHT_TO_LEFT)==0, 0, 100);
    RPU_SetLampState(LAMP_RIGHT_INLANE_BOTTOM, 0);
  } else {  
    RPU_SetLampState(LAMP_LEFT_RETURN_TOP, ReturnLaneStatus[CurrentPlayer] & 0x01);
    RPU_SetLampState(LAMP_RIGHT_RETURN_TOP, ReturnLaneStatus[CurrentPlayer] & 0x02);
    RPU_SetLampState(LAMP_LEFT_INLANE_BOTTOM, ReturnLaneStatus[CurrentPlayer] & 0x04);
    RPU_SetLampState(LAMP_RIGHT_INLANE_BOTTOM, ReturnLaneStatus[CurrentPlayer] & 0x08);
  }

}


void ShowLoopAndSpinnerLamps() {

  if (GameMode==GAME_MODE_SKILL_SHOT) {
    RPU_SetLampState(LAMP_SPINNER, 0);
    RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, 0);
    RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, 0);
    RPU_SetLampState(LAMP_ADV_BONUS_MULTIPLIER, 0);
  } else if (LastLeftInlane) {
    byte lampPhase = (CurrentTime/250)%3;
    if (CombosAchieved[CurrentPlayer]&COMBO_LEFT_TO_LOOP) lampPhase = 0;
    RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, lampPhase==1);
    RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, lampPhase==2);    
    RPU_SetLampState(LAMP_SPINNER, (CombosAchieved[CurrentPlayer]&COMBO_LEFT_TO_SPINNER)?0:1, 0, 100);
  } else if (LastRightInlane) {
    byte lampPhase = (CurrentTime/250)%3;
    if (CombosAchieved[CurrentPlayer]&COMBO_RIGHT_TO_LOOP) lampPhase = 0;
    RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, lampPhase==2);
    RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, lampPhase==1);    
    RPU_SetLampState(LAMP_SPINNER, 0);
  } else if (SpinnerFrenzyEndTime) {
    byte lampPhase = (CurrentTime/125)%3;
    RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, lampPhase==1);
    RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, lampPhase==2);
    RPU_SetLampState(LAMP_SPINNER, 1, 0, 100);
  } else {

    if ( SpinnerMultiballRunning ) {
      int spinnerFlash = ((CurrentTime/1000)%2) ? 250 : 50;
      RPU_SetLampState(LAMP_SPINNER, 1, 0, spinnerFlash);
    } else {
      boolean spinnerOn = (LoopSpinnerAlternation%2) || (SpinnerLitEndTime!=0);
      RPU_SetLampState(LAMP_SPINNER, spinnerOn, 0, SpinnerLitEndTime?100:0);
    }

    if ( LoopSpinnerAlternation>0 && (LoopSpinnerAlternation%2)==0 ) {
      byte lampPhase = (CurrentTime/100)%2;
      RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, lampPhase==0);
      RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, lampPhase==1);
    } else if (LoopMultiballRunning) {
      RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, 1, 0, LoopMultiballJackpotReady?0:250);
      RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, 1, 0, LoopMultiballJackpotReady?0:250);
    } else {
      RPU_SetLampState(LAMP_LEFT_ADV_MULTIPLIER, 0);
      RPU_SetLampState(LAMP_RIGHT_ADV_MULTIPLIER, 0);
    }
    
    RPU_SetLampState(LAMP_ADV_BONUS_MULTIPLIER, BonusXIncreaseAvailable[CurrentPlayer]?1:0, 0, (BonusXIncreaseAvailable[CurrentPlayer]>1)?225:0);
  }
  
}


void ShowDropTargetLamps() {

  if (GameMode==GAME_MODE_SKILL_SHOT) {
    for (byte count=0; count<3; count++) {
      RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+count, 0);
      RPU_SetLampState(LAMP_TOP_EXTRA_BALL+count, 0);
      RPU_SetLampState(LAMP_TOP_DROP_BANK+count, 0);
      RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+count, 0);
    }
    RPU_SetLampState(LAMP_123_FOR_SPECIAL, 0);
  } else if (LastRightInlane) {
    byte lampPhase = (CurrentTime/100)%3;
    if (CombosAchieved[CurrentPlayer]&COMBO_RIGHT_TO_UPPER) lampPhase = 3;
    RPU_SetLampState(LAMP_TOP_DROP_SPECIAL, lampPhase==0);
    RPU_SetLampState(LAMP_TOP_EXTRA_BALL, lampPhase==1);
    RPU_SetLampState(LAMP_TOP_DROP_BANK, lampPhase==2);
    RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE, 0);
    
    lampPhase = (CurrentTime/100)%3;
    if (CombosAchieved[CurrentPlayer]&COMBO_RIGHT_TO_2BANK) lampPhase = 3;
    RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+1, lampPhase==0);
    RPU_SetLampState(LAMP_TOP_EXTRA_BALL+1, lampPhase==1);
    RPU_SetLampState(LAMP_TOP_DROP_BANK+1, lampPhase==2);
    RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+1, 0);

    RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+2, 0);
    RPU_SetLampState(LAMP_TOP_EXTRA_BALL+2, 0);
    RPU_SetLampState(LAMP_TOP_DROP_BANK+2, 0);
    RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+2, 0);

    RPU_SetLampState(LAMP_123_FOR_SPECIAL, 0);
  } else if (LastLeftInlane) {
    RPU_SetLampState(LAMP_TOP_DROP_SPECIAL, 0);
    RPU_SetLampState(LAMP_TOP_EXTRA_BALL, 0);
    RPU_SetLampState(LAMP_TOP_DROP_BANK, 0);
    RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE, 0);
    
    byte lampPhase = (CurrentTime/100)%3;
    if (CombosAchieved[CurrentPlayer]&COMBO_LEFT_TO_2BANK) lampPhase = 3;
    RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+1, lampPhase==0);
    RPU_SetLampState(LAMP_TOP_EXTRA_BALL+1, lampPhase==1);
    RPU_SetLampState(LAMP_TOP_DROP_BANK+1, lampPhase==2);
    RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+1, 0);

    RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+2, 0);
    RPU_SetLampState(LAMP_TOP_EXTRA_BALL+2, 0);
    RPU_SetLampState(LAMP_TOP_DROP_BANK+2, 0);
    RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+2, 0);

    RPU_SetLampState(LAMP_123_FOR_SPECIAL, 0);
  } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_IN_ORDER) {
    for (byte count=0; count<3; count++) {
      RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+count, 0);
      RPU_SetLampState(LAMP_TOP_EXTRA_BALL+count, 0);
      RPU_SetLampState(LAMP_TOP_DROP_BANK+count, count==DropTargetStage[CurrentPlayer], 0, 250);
      RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+count, DropTargetStage[CurrentPlayer]>count);
    }
    RPU_SetLampState(LAMP_123_FOR_SPECIAL, 0);
  } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_FRENZY) {
    byte lampPhase = (CurrentTime/100)%3;
    for (byte count=0; count<3; count++) {
      RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+count, lampPhase==0);
      RPU_SetLampState(LAMP_TOP_EXTRA_BALL+count, lampPhase==1);
      if (DropTargetResetTime[count]==0) RPU_SetLampState(LAMP_TOP_DROP_BANK+count, lampPhase==2);
      else RPU_SetLampState(LAMP_TOP_DROP_BANK+count, DropTargetHurryLamp[count]);
      RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+count, count==lampPhase);
    }    
    RPU_SetLampState(LAMP_123_FOR_SPECIAL, 0);
  } else if (DropTargetMode[CurrentPlayer]>=DROP_TARGETS_JACKPOT) {
    byte lampPhase = (CurrentTime/150)%2;

    for (byte count=0; count<3; count++) {
      RPU_SetLampState(LAMP_TOP_DROP_SPECIAL+count, (DropTargetStage[CurrentPlayer]==count) && lampPhase==0);
      RPU_SetLampState(LAMP_TOP_EXTRA_BALL+count, (DropTargetStage[CurrentPlayer]==count) && lampPhase==0);
      if (DropTargetResetTime[count]==0) RPU_SetLampState(LAMP_TOP_DROP_BANK+count, (DropTargetStage[CurrentPlayer]==count) && lampPhase==1);
      else RPU_SetLampState(LAMP_TOP_DROP_BANK+count, DropTargetHurryLamp[count]);
      RPU_SetLampState(LAMP_TOP_DROP_BANK_COMPLETE+count, DropTargetStage[CurrentPlayer]>=count, 0, (DropTargetStage[CurrentPlayer]==count)?150:0);
    }

    int stageFlash = 500;
    if (DropTargetStage[CurrentPlayer]==1) stageFlash = 250;
    if (DropTargetStage[CurrentPlayer]==2) stageFlash = 125;
    RPU_SetLampState(LAMP_123_FOR_SPECIAL, 1, 0, stageFlash);
  }
  
}


void ShowShootAgainLamps() {

  if ( (BallFirstSwitchHitTime==0 && BallSaveNumSeconds) || (BallSaveEndTime && CurrentTime<BallSaveEndTime) ) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime!=0) msRemaining = BallSaveEndTime - CurrentTime;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
  } else {
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}




////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long StormCreditBIPDisplay;
byte StormDisplayMask = 0x00;

// This machine has a playfield display that changes the way that credits are shown...
void STORM23_SetDisplayCredits(int value, boolean displayOn=true, boolean showBothDigits=true) {

  unsigned long newDisplayValue = StormCreditBIPDisplay%10000;
  newDisplayValue += ((unsigned long)(value%100))*10000;  

  byte newMask = StormDisplayMask & ~0x06;
  if (displayOn) {
    newMask |= 0x04;
    if (value>9 || showBothDigits) newMask |= 0x02;
  }
  StormDisplayMask = newMask;
  StormCreditBIPDisplay = newDisplayValue;
  
  RPU_SetDisplay(4, StormCreditBIPDisplay, false, 0);
  RPU_SetDisplayBlank(4, newMask);
}

void STORM23_SetDisplayBallInPlay(int value, boolean displayOn=true, boolean showBothDigits=true) {

  unsigned long newDisplayValue = StormCreditBIPDisplay - (StormCreditBIPDisplay%100);
  newDisplayValue += ((unsigned long)(value%100));  

  byte newMask = StormDisplayMask & ~0x60;
  if (displayOn) {
    newMask |= 0x40;
    if (value>9 || showBothDigits) newMask |= 0x20;
  }
  StormDisplayMask = newMask;
  StormCreditBIPDisplay = newDisplayValue;
  
  RPU_SetDisplay(4, StormCreditBIPDisplay, false, 0);
  RPU_SetDisplayBlank(4, newMask);
}


void STORM23_SetPlayfieldDisplay(int value, boolean displayOn=true, boolean showBothDigits=true, boolean showTrailingZeroes=true) {

  unsigned long newDisplayValue = (StormCreditBIPDisplay%100) + ((StormCreditBIPDisplay/10000)%100)*10000;
  newDisplayValue += ((unsigned long)(value%100))*100;

  byte newMask = StormDisplayMask & ~0x18;
  if (displayOn) {
    newMask |= 0x10;
    if (value>9 || showBothDigits) newMask |= 0x08;
  }

  StormDisplayMask = newMask;
  StormCreditBIPDisplay = newDisplayValue;
  
  RPU_SetDisplay(4, StormCreditBIPDisplay, false, 0);
  RPU_SetDisplayBlank(4, newMask);

  RPU_SetContinuousSolenoidBit(showTrailingZeroes, CONTSOL_BONUS_BOARD_TOGGLE);
}

unsigned long BonusLastSetTime = 0;
unsigned long TimerLastSetTime = 0;
unsigned long SpinsLastSetTime = 0;
unsigned long NewTimerValue = 0;
byte LastBonusValueShown = 0;
byte LastTimerValueShown = 0;
byte LastSpinsValueShown = 0;

void ShowBonusInDisplay() {
  BonusLastSetTime = CurrentTime;
  LastBonusValueShown = 0;
}

void ShowTimerInDisplay(unsigned long timerValue) {
  TimerLastSetTime = CurrentTime;
  NewTimerValue = timerValue;
}

void ShowSpinsInDisplay(byte spinsValue) {
  if (spinsValue>99) LastSpinsValueShown = 99;
  else LastSpinsValueShown = spinsValue;
  SpinsLastSetTime = CurrentTime;
}

void UpdatePlayfieldDisplay() {

  if (SpinsLastSetTime) {
    if (CurrentTime>(SpinsLastSetTime+3000)) {
      SpinsLastSetTime = 0;
    } else {
      byte displayPhase = (CurrentTime/100)%2;
      STORM23_SetPlayfieldDisplay(LastSpinsValueShown, displayPhase, false, false);
    }
    LastBonusValueShown = 0;
  } else if (BonusLastSetTime) {
    if (CurrentTime>(BonusLastSetTime+2000)) {
      BonusLastSetTime = 0;
    } else {
      if (LastBonusValueShown==0) {
        LastBonusValueShown = Bonus[CurrentPlayer];
        STORM23_SetPlayfieldDisplay(LastBonusValueShown, true, false, true);
      }
    }
  } else if (TimerLastSetTime) {
    if (CurrentTime > (TimerLastSetTime+1000)) {
      TimerLastSetTime = 0;
    } else {
      byte newTimerValueToShow;
      if (NewTimerValue<1000) {
        newTimerValueToShow = NewTimerValue / 10;
      } else {
        newTimerValueToShow = NewTimerValue / 1000;
      }
      if (newTimerValueToShow!=LastTimerValueShown) {
        if (NewTimerValue<10000) {
          // For the last 10 seconds, always show the timer
          LastTimerValueShown = newTimerValueToShow;
          STORM23_SetPlayfieldDisplay(newTimerValueToShow, true, false, false);
        } else {
          // If greater than 10 seconds out, show timer alternating with bonus
          if ( ((NewTimerValue/1000)%4)==3 ) {
            STORM23_SetPlayfieldDisplay(Bonus[CurrentPlayer], true, false, true);
            LastTimerValueShown = 0;
          } else {
            LastTimerValueShown = newTimerValueToShow;
            STORM23_SetPlayfieldDisplay(LastTimerValueShown, true, false, false);
          }
        }
      }
      
    }
    LastBonusValueShown = 0;
  } else {
    if (LastBonusValueShown==0) {
      LastBonusValueShown = Bonus[CurrentPlayer];
      STORM23_SetPlayfieldDisplay(LastBonusValueShown, true, false, true);
    }
  }
}


unsigned long LastTimeScoreChanged = 0;
unsigned long LastFlashOrDash = 0;
unsigned long ScoreOverrideValue[4] = {0, 0, 0, 0};
byte LastAnimationSeed[4] = {0, 0, 0, 0};
byte AnimationStartSeed[4] = {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
byte ScoreAnimation[4] = {0, 0, 0, 0};
byte AnimationDisplayOrder[4] = {0, 1, 2, 3};
#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#define DISPLAY_OVERRIDE_ANIMATION_NONE     0
#define DISPLAY_OVERRIDE_ANIMATION_BOUNCE   1
#define DISPLAY_OVERRIDE_ANIMATION_FLUTTER  2
#define DISPLAY_OVERRIDE_ANIMATION_FLYBY    3
#define DISPLAY_OVERRIDE_ANIMATION_CENTER   4
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}


void OverrideScoreDisplay(byte displayNum, unsigned long value, byte animationType) {
  if (displayNum > 3) return;

  ScoreOverrideStatus |= (0x01 << displayNum);
  ScoreAnimation[displayNum] = animationType;
  ScoreOverrideValue[displayNum] = value;
  LastAnimationSeed[displayNum] = 255;
}


void ClearDisplayOverride(byte displayNum) {
  ScoreOverrideStatus &= ~(0x01 << displayNum);
  ScoreAnimation[displayNum] = DISPLAY_OVERRIDE_ANIMATION_NONE;
}

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount = 0; digitCount < numDigits; digitCount++) {
#if (RPU_OS_NUM_DIGITS==7)
    displayMask |= (0x40 >> digitCount);
#else
    displayMask |= (0x20 >> digitCount);
#endif
  }
  return displayMask;
}


void SetAnimationDisplayOrder(byte disp0, byte disp1, byte disp2, byte disp3) {
  AnimationDisplayOrder[0] = disp0;
  AnimationDisplayOrder[1] = disp1;
  AnimationDisplayOrder[2] = disp2;
  AnimationDisplayOrder[3] = disp3;
}


void ShowAnimatedValue(byte displayNum, unsigned long displayScore, byte animationType) {
  byte overrideAnimationSeed;
  byte displayMask = RPU_OS_ALL_DIGITS_MASK;

  byte numDigits = MagnitudeOfScore(displayScore);
  if (numDigits == 0) numDigits = 1;
  if (numDigits < (RPU_OS_NUM_DIGITS - 1) && animationType == DISPLAY_OVERRIDE_ANIMATION_BOUNCE) {
    // This score is going to be animated (back and forth)
    overrideAnimationSeed = (CurrentTime / 250) % (2 * RPU_OS_NUM_DIGITS - 2 * numDigits);
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {

      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      byte shiftDigits = (overrideAnimationSeed);
      if (shiftDigits >= ((RPU_OS_NUM_DIGITS + 1) - numDigits)) shiftDigits = (RPU_OS_NUM_DIGITS - numDigits) * 2 - shiftDigits;
      byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      //RPU_SetDisplayBlank(displayNum, 0x00);
      RPU_SetDisplay(displayNum, displayScore, false);
      RPU_SetDisplayBlank(displayNum, displayMask);
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_FLUTTER) {
    overrideAnimationSeed = CurrentTime / 50;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      displayMask = GetDisplayMask(numDigits);
      if (overrideAnimationSeed % 2) {
        displayMask &= 0x55;
      } else {
        displayMask &= 0xAA;
      }
      RPU_SetDisplay(displayNum, displayScore, false);
      RPU_SetDisplayBlank(displayNum, displayMask);
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_FLYBY) {
    overrideAnimationSeed = (CurrentTime / 75) % 256;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      if (LastAnimationSeed[displayNum] == 255) {
        AnimationStartSeed[displayNum] = overrideAnimationSeed;
      }
      LastAnimationSeed[displayNum] = overrideAnimationSeed;

      byte realAnimationSeed = overrideAnimationSeed - AnimationStartSeed[displayNum];
      if (overrideAnimationSeed < AnimationStartSeed[displayNum]) realAnimationSeed = (255 - AnimationStartSeed[displayNum]) + overrideAnimationSeed;

      if (realAnimationSeed > 34) {
        RPU_SetDisplayBlank(displayNum, 0x00);
        ScoreOverrideStatus &= ~(0x01 << displayNum);
      } else {
        int shiftDigits = (-6 * ((int)AnimationDisplayOrder[displayNum] + 1)) + realAnimationSeed;
        displayMask = GetDisplayMask(numDigits);
        if (shiftDigits < 0) {
          shiftDigits = 0 - shiftDigits;
          byte digitCount;
          for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
            displayScore /= 10;
            displayMask = displayMask << 1;
          }
        } else if (shiftDigits > 0) {
          byte digitCount;
          for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
            displayScore *= 10;
            displayMask = displayMask >> 1;
          }
        }
        RPU_SetDisplay(displayNum, displayScore, false);
        RPU_SetDisplayBlank(displayNum, displayMask);
      }
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_CENTER) {
    overrideAnimationSeed = CurrentTime / 250;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      byte shiftDigits = (RPU_OS_NUM_DIGITS - numDigits) / 2;

      byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      //RPU_SetDisplayBlank(displayNum, 0x00);
      RPU_SetDisplay(displayNum, displayScore, false);
      RPU_SetDisplayBlank(displayNum, displayMask);
    }
  } else {
    RPU_SetDisplay(displayNum, displayScore, true, 1);
  }

}

void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue = 0) {

  if (displayToUpdate == 0xFF) ScoreOverrideStatus = 0;
  byte displayMask = RPU_OS_ALL_DIGITS_MASK;
  unsigned long displayScore = 0;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime - LastTimeScoreChanged) / 125) % 16;
  if (scrollPhase != LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  for (byte scoreCount = 0; scoreCount < 4; scoreCount++) {

    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue == 0 && (ScoreOverrideStatus & (0x01 << scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      if (displayScore != DISPLAY_OVERRIDE_BLANK_SCORE) {
        ShowAnimatedValue(scoreCount, displayScore, ScoreAnimation[scoreCount]);
      } else {
        RPU_SetDisplayBlank(scoreCount, 0);
      }

    } else {
      boolean showingCurrentAchievement = false;
      // No override, update scores designated by displayToUpdate
      if (allScoresShowValue == 0) {
        displayScore = CurrentScores[scoreCount];
        displayScore += (CurrentAchievements[scoreCount] % 10);
        if (CurrentAchievements[scoreCount]) showingCurrentAchievement = true;
      }
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate == 0xFF || displayToUpdate == scoreCount || displayScore > RPU_OS_MAX_DISPLAY_SCORE || showingCurrentAchievement) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate == 0xFF && (scoreCount >= CurrentNumPlayers && CurrentNumPlayers != 0) && allScoresShowValue == 0) {
          RPU_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore > RPU_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled
          if ((CurrentTime - LastTimeScoreChanged) < 2000) {
            // show score for four seconds after change
            RPU_SetDisplay(scoreCount, displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1), false);
            byte blank = RPU_OS_ALL_DIGITS_MASK;
            if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
              blank &= ~(0x01 << (RPU_OS_NUM_DIGITS - 1));
            }
            RPU_SetDisplayBlank(scoreCount, blank);
          } else {
            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase < 11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);

              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase < RPU_OS_NUM_DIGITS) {
                displayMask = RPU_OS_ALL_DIGITS_MASK;
                for (byte scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0;
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits + scrollPhase) > 10) {
                byte numDigitsNeeded = (numDigits + scrollPhase) - 10;
                for (byte scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              RPU_SetDisplayBlank(scoreCount, displayMask);
              RPU_SetDisplay(scoreCount, displayScore);
            }
          }
        } else {
          if (flashCurrent && displayToUpdate == scoreCount) {
            unsigned long flashSeed = CurrentTime / 250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime / 250) % 2) == 0) RPU_SetDisplayBlank(scoreCount, 0x00);
              else RPU_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent && displayToUpdate == scoreCount) {
            unsigned long dashSeed = CurrentTime / 50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime / 60) % (2 * RPU_OS_NUM_DIGITS * 3);
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase < (2 * RPU_OS_NUM_DIGITS)) {
                displayMask = GetDisplayMask((numDigits == 0) ? 2 : numDigits);
                if (dashPhase < (RPU_OS_NUM_DIGITS + 1)) {
                  for (byte maskCount = 0; maskCount < dashPhase; maskCount++) {
                    displayMask &= ~(0x01 << maskCount);
                  }
                } else {
                  for (byte maskCount = (2 * RPU_OS_NUM_DIGITS); maskCount > dashPhase; maskCount--) {                    
                    byte firstDigit;
                    firstDigit = (0x20) << (RPU_OS_NUM_DIGITS - 6);                   
                    displayMask &= ~(firstDigit >> (maskCount - dashPhase - 1));
                  }
                }
                RPU_SetDisplay(scoreCount, displayScore);
                RPU_SetDisplayBlank(scoreCount, displayMask);
              } else {
                RPU_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
          } else {
            byte blank;
            blank = RPU_SetDisplay(scoreCount, displayScore, false, 2);
            if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
              blank &= ~(0x01 << (RPU_OS_NUM_DIGITS - 1));
            }
            RPU_SetDisplayBlank(scoreCount, blank);
          }
        }
      } // End if this display should be updated
    } // End on non-overridden
  } // End loop on scores

}

void ShowFlybyValue(byte numToShow, unsigned long timeBase) {
  byte shiftDigits = (CurrentTime - timeBase) / 120;
  byte rightSideBlank = 0;

  unsigned long bigVersionOfNum = (unsigned long)numToShow;
  for (byte count = 0; count < shiftDigits; count++) {
    bigVersionOfNum *= 10;
    rightSideBlank /= 2;
    if (count > 2) rightSideBlank |= 0x20;
  }
  bigVersionOfNum /= 1000;

  byte curMask = RPU_SetDisplay(CurrentPlayer, bigVersionOfNum, false, 0);
  if (bigVersionOfNum == 0) curMask = 0;
  RPU_SetDisplayBlank(CurrentPlayer, ~(~curMask | rightSideBlank));
}

void StartScoreAnimation(unsigned long scoreToAnimate, boolean playTick = true) {
  if (ScoreAdditionAnimation != 0) {
    //CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
  }
  ScoreAdditionAnimation += scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
  if (playTick) {
    PlayScoreAnimationTick = 10000;
    if (scoreToAnimate<=10000) PlayScoreAnimationTick = 2500;
    else if (scoreToAnimate<=25000) PlayScoreAnimationTick = 5000;
    else if (scoreToAnimate<=50000) PlayScoreAnimationTick = 7500;
    else if (scoreToAnimate<=100000) PlayScoreAnimationTick = 15000;
    else if (scoreToAnimate<=1000000) PlayScoreAnimationTick = 50000;
    else PlayScoreAnimationTick = 100000;
  } else {
    PlayScoreAnimationTick = 1;
  }
}


////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  RPU_SetDisplay(CurrentNumPlayers - 1, 0, true, 2);
//  RPU_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

//  RPU_SetLampState(LAMP_HEAD_1_PLAYER, CurrentNumPlayers==1, 0, 500);
//  RPU_SetLampState(LAMP_HEAD_2_PLAYERS, CurrentNumPlayers==2, 0, 500);
//  RPU_SetLampState(LAMP_HEAD_3_PLAYERS, CurrentNumPlayers==3, 0, 500);
//  RPU_SetLampState(LAMP_HEAD_4_PLAYERS, CurrentNumPlayers==4, 0, 500);

  if (!FreePlayMode) {
    Credits -= 1;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  }
  if (CurrentNumPlayers==1) Audio.StopAllAudio();
  QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1 + (CurrentNumPlayers - 1), 10);

  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}


unsigned short ChuteAuditByte[] = {RPU_CHUTE_1_COINS_START_BYTE, RPU_CHUTE_2_COINS_START_BYTE, RPU_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum>2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) {
      PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
      //RPU_PushToSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_TIME, true); 
    }
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  } else {
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true);
  }

}

byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit==SW_COIN_2) chuteNum = 1;
  else if (switchHit==SW_COIN_3) chuteNum = 2;
  return chuteNum;
}

boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum>2) return false;
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse=0; chuteNumToUse<=chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse)==cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_COIN_DROP_1+(CurrentTime%3));

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse]==cpcCoins) {
    if (cpcCredits>cpcCoins) AddCredit(cpcCredits - (coinProgressBefore));
    else AddCredit(cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits>cpcCoins) {
      AddCredit(1);
      creditAdded = true;
    } else {
    }
  }

  return creditAdded;
}


void AddSpecialCredit() {
  AddCredit(false, 1);
  RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_TIME, CurrentTime, true);
  RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += SpecialValue * PlayfieldMultiplier;
  } else {
    AddSpecialCredit();
  }
}

boolean AwardExtraBall() {
  if (ExtraBallCollected) return false;
  ExtraBallCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ExtraBallValue * PlayfieldMultiplier;
  } else {
    SamePlayerShootsAgain = true;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    QueueNotification(SOUND_EFFECT_VP_EXTRA_BALL, 8);
  }
  return true;
}


void IncreasePlayfieldMultiplier(unsigned long duration) {

  if (HoldoverAwards[CurrentPlayer]&HOLDOVER_TIMER_MULTIPLIER) duration *= 2;
  
  if (PlayfieldMultiplierExpiration) PlayfieldMultiplierExpiration += duration;
  else PlayfieldMultiplierExpiration = CurrentTime + duration;
  PlayfieldMultiplier += 1;
  if (PlayfieldMultiplier > 7) {
    PlayfieldMultiplier = 7;
  } else {
//    QueueNotification(SOUND_EFFECT_VP_RETURN_TO_1X + (PlayfieldMultiplier - 1), 1);
  }
}


void StartJailBreakMultiball() {
  IncreasePlayfieldMultiplier(60000);
  JailBreakMultiballRunning = true;
  NumberOfBallsInPlay += 1;
  PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_MULTIBALL);
  QueueNotification(SOUND_EFFECT_VP_JAILBREAK_MULTIBALL, 10);
  unsigned int startingHits = StandupHits[CurrentPlayer];
  StandupHits[CurrentPlayer] = STANDUPS_ALL;
  UpdateLocksBasedOnStandups(startingHits);
}



unsigned long UpperLockSwitchDownTime[2] = {0, 0};
unsigned long UpperLockSwitchUpTime[2] = {0, 0};
unsigned long UpperLockLastChecked = 0;
unsigned long MachineLockDiscrepancyTime = 0;
boolean UpperLockSwitchState[2] = {false, false};

byte InitializeMachineLocksBasedOnSwitches() {
  byte returnLocks = 0;

  if (RPU_ReadSingleSwitchState(SW_SAUCER_1)) {
    returnLocks |= LOCK_1_ENGAGED;
    // Also update the UpperLockSwitchState variable
    UpperLockSwitchState[0] = true;
  }
  if (RPU_ReadSingleSwitchState(SW_SAUCER_2)) {
    returnLocks |= LOCK_2_ENGAGED;
    // Also update the UpperLockSwitchState variable
    UpperLockSwitchState[1] = true;
  }

  if (DEBUG_MESSAGES) {
    char buf[256];
    sprintf(buf, "Initializing Machine Locks = 0x%04X\n", returnLocks);
    Serial.write(buf);
  }
  
  return returnLocks;
}

void UpdateLockStatus() {
  boolean lockSwitchDownTransition;
  boolean lockSwitchUpTransition;

  for (byte count=0; count<2; count++) {
    lockSwitchDownTransition = false;
    lockSwitchUpTransition = false;

    if (RPU_ReadSingleSwitchState(SW_SAUCER_1-count)) {
      UpperLockSwitchUpTime[count] = 0;
      if (UpperLockSwitchDownTime[count]==0) {
        UpperLockSwitchDownTime[count] = CurrentTime;
      } else if (CurrentTime > (UpperLockSwitchDownTime[count] + 250)) {
        lockSwitchDownTransition = true;
      }
    } else {
      UpperLockSwitchDownTime[count] = 0;
      if (UpperLockSwitchUpTime[count]==0) {
        UpperLockSwitchUpTime[count] = CurrentTime;
      } else if (CurrentTime > (UpperLockSwitchUpTime[count] + 250)) {
        lockSwitchUpTransition = true;
      }
    }

    if (lockSwitchUpTransition && UpperLockSwitchState[count]) {
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "ULS - saw up on %d (%ld)\n", count, CurrentTime);
        Serial.write(buf);
      }
      // if we used to be down & now we're up
      UpperLockSwitchState[count] = false;
      HandleSaucerUpSwitch(count);
    } else if (lockSwitchDownTransition && !UpperLockSwitchState[count]) {
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "ULS - saw down on %d (%ld)\n", count, CurrentTime);
        Serial.write(buf);
      }
      // if we used to be up & now we're down
      UpperLockSwitchState[count] = true;
      HandleSaucerDownSwitch(count);
    }
  }

  if (UpperLockLastChecked==0 || CurrentTime>(UpperLockLastChecked+500) || MachineLockDiscrepancyTime) {
    UpperLockLastChecked = CurrentTime;
    byte curUpperLock = 0;
    if (UpperLockSwitchState[0] && !Saucer1ScheduledEject) curUpperLock |= LOCK_1_ENGAGED;
    if (UpperLockSwitchState[1] && !Saucer2ScheduledEject) curUpperLock |= LOCK_2_ENGAGED;

    if ( (MachineLocks&LOCKS_ENGAGED_MASK)!=curUpperLock ) {
      if (MachineLockDiscrepancyTime==0) {
        MachineLockDiscrepancyTime = CurrentTime;
      } else if (CurrentTime>(MachineLockDiscrepancyTime+1000)) {

        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "** MachineLocks out of sync (0x%02X versus 0x%02X)\n", MachineLocks, curUpperLock);
          Serial.write(buf);
        }
        
        // If "MachineLocks" has been out of sync
        // for 1000ms, we should re-sync it
        // to the switches
        for (byte count=0; count<2; count++) UpperLockSwitchState[count] = false;
//        MachineLocks &= ~LOCKS_ENGAGED_MASK;
        MachineLocks = InitializeMachineLocksBasedOnSwitches();
        NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
        NumberOfBallsInPlay = TotalBallsLoaded - (NumberOfBallsLocked + CountBallsInTrough());

        // Adjust player locks as necessary
        UpdatePlayerLocks();

        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Now ML=%d, BL=%d, BIP=%d, T=%d, PL=%d\n", MachineLocks, NumberOfBallsLocked, NumberOfBallsInPlay, CountBallsInTrough(), PlayerLockStatus[CurrentPlayer]);
          Serial.write(buf);
        }
      
      }
    } else {
      MachineLockDiscrepancyTime = 0;
    }
  }
}


void UpdatePlayerLocks() {
  byte curLockEngagedStatus = PlayerLockStatus[CurrentPlayer] & LOCKS_ENGAGED_MASK;
  if (curLockEngagedStatus) {
    // Check to see if a lock has been stolen
    if ( (curLockEngagedStatus & MachineLocks)!=curLockEngagedStatus ) {
      byte lockToCheck = LOCK_1_ENGAGED;
      byte lockAvail = LOCK_1_AVAILABLE;
      for (byte count=0; count<2; count++) {
        if ( (curLockEngagedStatus&lockToCheck) && !(MachineLocks&lockToCheck) ) {
          // If a lock has been stolen, move it from locked to available
          PlayerLockStatus[CurrentPlayer] &= ~lockToCheck;
          PlayerLockStatus[CurrentPlayer] |= lockAvail;
        }
        lockToCheck *= 2;
        lockAvail *= 2;
      }
    }
  }

  byte curLockReadyStatus = PlayerLockStatus[CurrentPlayer] & LOCKS_AVAILABLE_MASK;
  if (curLockReadyStatus) {
    // Check to see if a ready lock has been filled by someone else
    if ( (curLockReadyStatus & LOCK_1_AVAILABLE) && (MachineLocks & LOCK_1_ENGAGED) ) {
      QueueNotification(SOUND_EFFECT_VP_BALL_1_LOCKED, 8);
      PlayerLockStatus[CurrentPlayer] |= LOCK_1_ENGAGED;
      PlayerLockStatus[CurrentPlayer] &= ~LOCK_1_AVAILABLE;
    }

    if ( (curLockReadyStatus & LOCK_2_AVAILABLE) && (MachineLocks & LOCK_2_ENGAGED) ) {
      QueueNotification(SOUND_EFFECT_VP_BALL_2_LOCKED, 8);
      PlayerLockStatus[CurrentPlayer] |= LOCK_2_ENGAGED;
      PlayerLockStatus[CurrentPlayer] &= ~LOCK_2_AVAILABLE;
    }  
  }
}

void LockBall(byte lockIndex, boolean updateNumberOfBallsLocked = true) {
  PlayerLockStatus[CurrentPlayer] &= ~(LOCK_1_AVAILABLE<<lockIndex);
  PlayerLockStatus[CurrentPlayer] |= (LOCK_1_ENGAGED<<lockIndex);
  MachineLocks |= (LOCK_1_ENGAGED<<lockIndex);
  if (updateNumberOfBallsLocked) NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
}

void ReleaseLockedBall(byte lockIndex) {

  if (PlayerLockStatus[CurrentPlayer]&(LOCK_1_ENGAGED<<lockIndex)) {
    // This ball can be released
    NumberOfBallsLocked -= 1;
    NumberOfBallsInPlay += 1;
    
    if (lockIndex==0) RPU_PushToSolenoidStack(SOL_SAUCER_1, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
    else RPU_PushToSolenoidStack(SOL_SAUCER_2, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);

    PlayerLockStatus[CurrentPlayer] &= ~(LOCK_1_ENGAGED<<lockIndex);
    MachineLocks &= ~(LOCK_1_ENGAGED<<lockIndex);

    if (DEBUG_MESSAGES) {
      char buf[256];
      sprintf(buf, "Releasing %d - Machine Locks = 0x%04X\n", lockIndex, MachineLocks);
      Serial.write(buf);
    }  
  }
}

void EjectUnlockedBall(byte saucerIndex) {

  if (MachineLocks & (LOCK_1_ENGAGED<<saucerIndex)) {
    char buf[128];
    sprintf(buf, "** eject requested, but %d is machine locked (0x%02X)\n", saucerIndex, MachineLocks);
    Serial.write(buf);
    return;
  }
  
  if (saucerIndex==0) {
    RPU_PushToSolenoidStack(SOL_SAUCER_1, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
  } else {
    RPU_PushToSolenoidStack(SOL_SAUCER_2, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
  }

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Eject %d\n", saucerIndex);
    Serial.write(buf);
  }

}



#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
unsigned long SoundSettingTimeout = 0;
unsigned long AdjustmentScore;



int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    Audio.StopAllAudio();
    RPU_TurnOffAllLamps();
    int modeMapping = SelfTestStateToCalloutMap[-1 - curState];
    Audio.PlaySound((unsigned short)modeMapping, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
  } else {
    if (SoundSettingTimeout && CurrentTime>SoundSettingTimeout) {
      SoundSettingTimeout = 0;
      Audio.StopAllAudio();
    }
  }

  // Any state that's greater than MACHINE_STATE_TEST_DONE is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_DONE) {
    byte cpcSelection = 0xFF;
    byte chuteNum = 0xFF;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1) chuteNum = 0;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2) chuteNum = 1;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) chuteNum = 2;
    if (chuteNum!=0xFF) cpcSelection = GetCPCSelection(chuteNum);
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);

    if (curState<=MACHINE_STATE_TEST_SCORE_LEVEL_1) {
      STORM23_SetDisplayCredits(0, false);
      STORM23_SetDisplayBallInPlay(MACHINE_STATE_TEST_SOUNDS-curState);
    } else if (curState!=MACHINE_STATE_TEST_DISPLAYS) {
      STORM23_SetDisplayCredits(0 - curState, true);
      STORM23_SetDisplayBallInPlay(0, false);      
    }
    
    if (chuteNum!=0xFF) {
      if (cpcSelection != GetCPCSelection(chuteNum)) {
        byte newCPC = GetCPCSelection(chuteNum);
        Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_SELF_TEST_CPC_START+newCPC, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
      }
    }
  } else {
    byte curSwitch = RPU_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      if (RPU_GetUpDownSwitchState()) returnState -= 1;
      else returnState += 1;
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
        RPU_SetDisplay(count, 0);
        RPU_SetDisplayBlank(count, 0x00);
      }

      STORM23_SetDisplayCredits(0, false);
      STORM23_SetDisplayBallInPlay(MACHINE_STATE_TEST_SOUNDS - curState);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SOUND_SELECTOR:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 6;
          AdjustmentValues[2] = 7;
          AdjustmentValues[3] = 8;
          AdjustmentValues[4] = 9;
          CurrentAdjustmentByte = &SoundSelector;
          CurrentAdjustmentStorageByte = EEPROM_SOUND_SELECTOR_BYTE;
          break;
        case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &MusicVolume;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SFX_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &SoundEffectsVolume;
          CurrentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &CalloutsVolume;
          CurrentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 3;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 99;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;
        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_UL;
          break;
        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_UL;
          break;
        case MACHINE_STATE_ADJUST_RESET_HOLD_TIME:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &TimeRequiredToResetGame;
          CurrentAdjustmentStorageByte = EEPROM_RESET_HOLD_TIME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SOUND_TRACK:
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &SoundTrackNum;
          CurrentAdjustmentStorageByte = EEPROM_SOUND_TRACK_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SAUCER_EJECT:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &SaucerEjectStrength;
          CurrentAdjustmentStorageByte = EEPROM_SAUCER_EJECT_BYTE;
          break;
        case MACHINE_STATE_ADJUST_MULTIBALL_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          CurrentAdjustmentByte = &MultiballBallSave;
          CurrentAdjustmentStorageByte = EEPROM_MB_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SPINS_UNTIL_MB:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[0] = 25;
          AdjustmentValues[1] = 50;
          AdjustmentValues[2] = 75;
          AdjustmentValues[3] = 100;
          AdjustmentValues[4] = 125;
          CurrentAdjustmentByte = &SpinsUntilMultiball;
          CurrentAdjustmentStorageByte = EEPROM_SPINS_UNTIL_MB_BYTE;
          break;
        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }
    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT) ) {
        byte curVal = *CurrentAdjustmentByte;

        if (RPU_GetUpDownSwitchState()) {
          curVal += 1;
          if (curVal > AdjustmentValues[1]) {
            if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
            else {
              if (curVal > 99) curVal = AdjustmentValues[0];
              else curVal = 99;
            }
          }
        } else {
          if (curVal==AdjustmentValues[0]) {            
            if (AdjustmentType==ADJ_TYPE_MIN_MAX_DEFAULT) curVal = 99;
            else curVal = AdjustmentValues[1];
          } else {
            curVal -= 1;
          }
        }

        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);

        if (curState==MACHINE_STATE_ADJUST_SOUND_SELECTOR) {
          Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START+curVal, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
        } else if (curState==MACHINE_STATE_ADJUST_MUSIC_VOLUME) {
          if (SoundSettingTimeout) Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_BACKGROUND_SONG_1, AUDIO_PLAY_TYPE_WAV_TRIGGER, curVal);
          Audio.SetMusicVolume(curVal);
          SoundSettingTimeout = CurrentTime+5000;
        } else if (curState==MACHINE_STATE_ADJUST_SFX_VOLUME) {
          if (SoundSettingTimeout) Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_BONUS_COLLECT, AUDIO_PLAY_TYPE_WAV_TRIGGER, curVal);
          Audio.SetSoundFXVolume(curVal);
          SoundSettingTimeout = CurrentTime+5000;
        } else if (curState==MACHINE_STATE_ADJUST_CALLOUTS_VOLUME) {
          if (SoundSettingTimeout) Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_VP_JACKPOT, AUDIO_PLAY_TYPE_WAV_TRIGGER, curVal);
          Audio.SetNotificationsVolume(curVal);
          SoundSettingTimeout = CurrentTime+3000;
        } else if (curState==MACHINE_STATE_ADJUST_RESET_HOLD_TIME) {
          Audio.StopAllAudio();
          if (curVal==99) {
            Audio.PlaySound(SOUND_EFFECT_SELF_TEST_CRB_OPTIONS_START, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          } else {
            Audio.PlaySound(SOUND_EFFECT_SELF_TEST_CRB_OPTIONS_START+curVal+1, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          }
        } else if (curState==MACHINE_STATE_ADJUST_MULTIBALL_BALL_SAVE) {
          if (curVal==1) {
            Audio.PlaySound(SOUND_EFFECT_SELF_TEST_MBBS_WARNING, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          }
        }
        
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        boolean upDownState = RPU_GetUpDownSwitchState();
        for (valCount = 0; valCount < (NumAdjustmentValues); valCount++) {
          if (curVal == AdjustmentValues[valCount]) {
            if (upDownState) {
              if (valCount<(NumAdjustmentValues-1)) newIndex = valCount + 1;
            } else {
              if (valCount>0) newIndex = valCount - 1;
            }
          }
        }

        if (curState==MACHINE_STATE_ADJUST_SOUND_SELECTOR) {
          Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START+AdjustmentValues[newIndex], AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
        }
        
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        if (RPU_GetUpDownSwitchState()) curVal += 5000;
        else if (curVal>=5000) curVal -= 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) RPU_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      RPU_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      RPU_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
    //    DecodeDIPSwitchParameters();
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);
    ReadStoredParameters();
  }

  return returnState;
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////
void PlayBackgroundSong(unsigned int songNum) {

  if (MusicVolume==0) return;

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Music = %d\n", songNum + ((unsigned int)SoundTrackNum-1)*100);
    Serial.write(buf);
  }

  Audio.PlayBackgroundSong(songNum + ((unsigned int)SoundTrackNum-1)*100);
}


unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(unsigned int soundEffectNum) {

  if (MachineState==MACHINE_STATE_INIT_GAMEPLAY) return;
  Audio.PlaySound(soundEffectNum, AUDIO_PLAY_TYPE_WAV_TRIGGER);

}


void QueueNotification(unsigned int soundEffectNum, byte priority) {
  if (CalloutsVolume==0) return;
  if (SoundSelector<3 || SoundSelector==4 || SoundSelector==7 || SoundSelector==9) return; 

  Audio.QueuePrioritizedNotification(soundEffectNum, 0, priority, CurrentTime);
}


void AlertPlayerUp(byte playerNum) {
//  (void)playerNum;
//  QueueNotification(SOUND_EFFECT_VP_PLAYER, 1);
  QueueNotification(SOUND_EFFECT_VP_PLAYER_ONE_UP + playerNum, 1);
//  QueueNotification(SOUND_EFFECT_VP_LAUNCH_WHEN_READY, 1); 
}





////////////////////////////////////////////////////////////////////////////
//
//  Diagnostics Mode
//
////////////////////////////////////////////////////////////////////////////

int RunDiagnosticsMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {

/*
    char buf[256];
    boolean errorSeen;

    Serial.write("Testing Volatile RAM at IC13 (0x0000 - 0x0080): writing & reading... ");
    Serial.write("3 ");
    delay(500);
    Serial.write("2 ");
    delay(500);
    Serial.write("1 \n");
    delay(500);
    errorSeen = false;
    for (byte valueCount=0; valueCount<0xFF; valueCount++) {
      for (unsigned short address=0x0000; address<0x0080; address++) {
        RPU_DataWrite(address, valueCount);
      }
      for (unsigned short address=0x0000; address<0x0080; address++) {
        byte readValue = RPU_DataRead(address);
        if (readValue!=valueCount) {
          sprintf(buf, "Write/Read failure at address=0x%04X (expected 0x%02X, read 0x%02X)\n", address, valueCount, readValue);
          Serial.write(buf);
          errorSeen = true;
        }
        if (errorSeen) break;
      }
      if (errorSeen) break;
    }
    if (errorSeen) {
      Serial.write("!!! Error in Volatile RAM\n");
    }

    Serial.write("Testing Volatile RAM at IC16 (0x0080 - 0x0100): writing & reading... ");
    Serial.write("3 ");
    delay(500);
    Serial.write("2 ");
    delay(500);
    Serial.write("1 \n");
    delay(500);
    errorSeen = false;
    for (byte valueCount=0; valueCount<0xFF; valueCount++) {
      for (unsigned short address=0x0080; address<0x0100; address++) {
        RPU_DataWrite(address, valueCount);
      }
      for (unsigned short address=0x0080; address<0x0100; address++) {
        byte readValue = RPU_DataRead(address);
        if (readValue!=valueCount) {
          sprintf(buf, "Write/Read failure at address=0x%04X (expected 0x%02X, read 0x%02X)\n", address, valueCount, readValue);
          Serial.write(buf);
          errorSeen = true;
        }
        if (errorSeen) break;
      }
      if (errorSeen) break;
    }
    if (errorSeen) {
      Serial.write("!!! Error in Volatile RAM\n");
    }
    
    // Check the CMOS RAM to see if it's operating correctly
    errorSeen = false;
    Serial.write("Testing CMOS RAM: writing & reading... ");
    Serial.write("3 ");
    delay(500);
    Serial.write("2 ");
    delay(500);
    Serial.write("1 \n");
    delay(500);
    for (byte valueCount=0; valueCount<0x10; valueCount++) {
      for (unsigned short address=0x0100; address<0x0200; address++) {
        RPU_DataWrite(address, valueCount);
      }
      for (unsigned short address=0x0100; address<0x0200; address++) {
        byte readValue = RPU_DataRead(address);
        if ((readValue&0x0F)!=valueCount) {
          sprintf(buf, "Write/Read failure at address=0x%04X (expected 0x%02X, read 0x%02X)\n", address, valueCount, (readValue&0x0F));
          Serial.write(buf);
          errorSeen = true;
        }
        if (errorSeen) break;
      }
      if (errorSeen) break;
    }
    
    if (errorSeen) {
      Serial.write("!!! Error in CMOS RAM\n");
    }
    
    
    // Check the ROMs
    Serial.write("CMOS RAM dump... ");
    Serial.write("3 ");
    delay(500);
    Serial.write("2 ");
    delay(500);
    Serial.write("1 \n");
    delay(500);
    for (unsigned short address=0x0100; address<0x0200; address++) {
      if ((address&0x000F)==0x0000) {
        sprintf(buf, "0x%04X:  ", address);
        Serial.write(buf);
      }
//      RPU_DataWrite(address, address&0xFF);
      sprintf(buf, "0x%02X ", RPU_DataRead(address));
      Serial.write(buf);
      if ((address&0x000F)==0x000F) {
        Serial.write("\n");
      }
    }

*/

//    RPU_EnableSolenoidStack();
//    RPU_SetDisableFlippers(false);
        
  }

  return returnState;
}



////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractDisplayRampStart = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;
byte CurrentAttractAnimationStep = 255;

unsigned long Saucer1Down = 0;
unsigned long Saucer2Down = 0;
unsigned long ThorPlayTime = 0;


int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    //PlaySoundEffect(SOUND_EFFECT_STOP_BACKGROUND);
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);

    MachineLocks = 0;
    ThorPlayTime = CurrentTime + 30000;
    STORM23_SetPlayfieldDisplay(99, true, true, true);
    ShowPlayerScores(0xFF, false, false);
  }

  if (CurrentTime>ThorPlayTime) {
    ThorPlayTime = CurrentTime + 360000;
    PlaySoundEffect(SOUND_EFFECT_MACHINE_START);
  }

  
  if (RPU_ReadSingleSwitchState(SW_SAUCER_1)) {
    if (Saucer1Down && CurrentTime>(Saucer1Down+250)) {
      if (Saucer1EjectTime==0 || CurrentTime > (Saucer1EjectTime+2500)) {
        if (DEBUG_MESSAGES) {
          Serial.write("kicking saucker 1 a\n");
        }
        RPU_PushToSolenoidStack(SOL_SAUCER_1, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
        Saucer1EjectTime = CurrentTime;
      }
    } else {
      if (Saucer1Down==0) Saucer1Down = CurrentTime;
    }
  } else {
    Saucer1Down = 0;
  }

  if (RPU_ReadSingleSwitchState(SW_SAUCER_2)) {
    if (Saucer2Down && CurrentTime>(Saucer2Down+250)) {
      if (Saucer2EjectTime==0 || CurrentTime > (Saucer2EjectTime+2500)) {
        if (DEBUG_MESSAGES) {
          Serial.write("kicking saucker 2 a\n");
        }
        RPU_PushToSolenoidStack(SOL_SAUCER_2, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
        Saucer2EjectTime = CurrentTime;
      }
    } else {
      if (Saucer2Down==0) Saucer2Down = CurrentTime;
    }
  } else {
    Saucer2Down = 0;
  }

  MoveBallFromOutholeToRamp();
 
  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      ShowPlayerScores(0xFF, false, false);
      STORM23_SetDisplayCredits(Credits, !FreePlayMode);
      STORM23_SetDisplayBallInPlay(0, true);
      STORM23_SetPlayfieldDisplay(99, true, true, true);
    }
  } else if ((CurrentTime / 8000) % 2 == 0) {

    boolean playfieldDisplayPhase = ((CurrentTime/250)%2) ? true : false;
    STORM23_SetPlayfieldDisplay(99, playfieldDisplayPhase, true, playfieldDisplayPhase);

    if (AttractLastHeadMode != 2) {
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 1, 0, 250);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 5);

  if (attractPlayfieldPhase != AttractLastPlayfieldMode) {
    RPU_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase == 2) GameMode = GAME_MODE_SKILL_SHOT;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
    AttractLastLadderBonus = 1;
    AttractLastLadderTime = CurrentTime;
  }

  byte animStep = ((CurrentTime/25)%LAMP_ANIMATION_STEPS);
  if (animStep!=CurrentAttractAnimationStep) {
    CurrentAttractAnimationStep = animStep;
    ShowLampAnimationSingleStep(attractPlayfieldPhase, animStep);
  }

  byte switchHit;
  while ((switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_LAMPS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(unsigned short intToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}


void SetGameMode(byte newGameMode) {
  GameMode = newGameMode;
  GameModeStartTime = 0;
  GameModeEndTime = 0;
}


byte LastBallsInTrough = 0;
byte LastSteadyBallsInTrough = 0;
unsigned long LastTroughSteadyTime = 0;

byte CountBallsInTrough() {
  // RPU_ReadSingleSwitchState(SW_OUTHOLE) + 
  
  byte numBalls = RPU_ReadSingleSwitchState(SW_TROUGH_1) + 
                  RPU_ReadSingleSwitchState(SW_TROUGH_2) +
                  RPU_ReadSingleSwitchState(SW_TROUGH_3);

  if (numBalls==LastBallsInTrough) {
    if (LastTroughSteadyTime) {
      if (CurrentTime>(LastTroughSteadyTime+250)) {
        LastSteadyBallsInTrough = numBalls;
        return numBalls;
      }
    } else {
      LastTroughSteadyTime = CurrentTime;
    }
  } else {
    LastTroughSteadyTime = 0;
  }
  
  LastBallsInTrough = numBalls;

  return LastSteadyBallsInTrough;
}



void AddToBonus(byte bonus) {
  Bonus[CurrentPlayer] += bonus;
  if (Bonus[CurrentPlayer]>MAX_DISPLAY_BONUS) {
    Bonus[CurrentPlayer] = MAX_DISPLAY_BONUS;
  } else {
    ShowBonusInDisplay();
  }
  BonusChangedTime = CurrentTime;
}



void IncreaseBonusX() {
  boolean soundPlayed = false;
  if (BonusX[CurrentPlayer] < 10) {
    BonusX[CurrentPlayer] += 1;
    BonusXAnimationStart = CurrentTime;

    if (BonusX[CurrentPlayer] == 9) {
      BonusX[CurrentPlayer] = 10;
//      QueueNotification(SOUND_EFFECT_VP_BONUSX_MAX, 2);
    } else {
//      QueueNotification(SOUND_EFFECT_VP_BONUS_X_INCREASED, 1);
    }
  }

  if (!soundPlayed) {
    //    PlaySoundEffect(SOUND_EFFECT_BONUS_X_INCREASED);
  }

}

unsigned long GameStartNotificationTime = 0;
boolean WaitForBallToReachOuthole = false;
unsigned long UpperBallEjectTime = 0;

int InitGamePlay(boolean curStateChanged) {

  if (curStateChanged) {
    Audio.StopAllAudio();
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetDisableFlippers(true);
    STORM23_SetPlayfieldDisplay(0, false, false, false);
    GameStartNotificationTime = CurrentTime;
    STORM23_SetDisplayBallInPlay(1);    
        
    // Reset displays & game state variables
    for (int count = 0; count < 4; count++) {
      // Initialize game-specific variables
      BonusX[count] = 1;
      PlayerLockStatus[count] = 0;
      CombosAchieved[count] = 0;
      RPU_SetDisplayBlank(count, 0x00);
      CurrentScores[count] = 0;
      DropTargetMode[count] = 0;
      DropTargetStage[count] = 0;
      AlternationMode[count] = 0;
      ReturnLaneStatus[count] = 0;
      StandupHits[count] = 0;
      StandupLevel[count] = 0;
      LoopMulitballGoal[count] = 3;
      SpinnerMaxGoal[count] = SpinsUntilMultiball;
      HoldoverAwards[count] = 0;  
      BonusXIncreaseAvailable[CurrentPlayer] = 0;
      StormMultiballReady[CurrentPlayer] = false;
      FeaturesCompleted[CurrentPlayer] = 0;
    }
    ShowPlayerScores(0xFF, false, false);
    
  }

  if (RPU_ReadSingleSwitchState(SW_SAUCER_1)) {
    if (Saucer1EjectTime==0 || CurrentTime > (Saucer1EjectTime+2500)) {
      if (DEBUG_MESSAGES) {
        Serial.write("kicking saucker 1 b\n");
      }
      RPU_PushToSolenoidStack(SOL_SAUCER_1, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
      Saucer1EjectTime = CurrentTime;
    }
  }

  if (RPU_ReadSingleSwitchState(SW_SAUCER_2)) {
    if (Saucer2EjectTime==0 || CurrentTime > (Saucer2EjectTime+2500)) {
      if (DEBUG_MESSAGES) {
        Serial.write("kicking saucker 2 b\n");
      }
      RPU_PushToSolenoidStack(SOL_SAUCER_2, SAUCER_KICKOUT_TIME + SaucerEjectStrength*3, true);
      Saucer2EjectTime = CurrentTime;
    }
  }

  NumberOfBallsInPlay = 0;
  UpdateLockStatus();
  STORM23_SetDisplayBallInPlay(1, (CurrentTime/250)%2);
  
  // Should always be zero
  NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);  

  if (NumberOfBallsLocked) {
    // We need to come back to this function because something is wrong
    return MACHINE_STATE_INIT_GAMEPLAY;
  }

  if (CountBallsInTrough()<TotalBallsLoaded) {

    MoveBallFromOutholeToRamp();

    if (CurrentTime>(GameStartNotificationTime+5000)) {
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "(ball missing) %d balls in trough (%d locked, %d in play)\n", CountBallsInTrough(), NumberOfBallsLocked, NumberOfBallsInPlay);
        Serial.write(buf);
      }
      GameStartNotificationTime = CurrentTime;
      QueueNotification(SOUND_EFFECT_VP_BALL_MISSING, 10);
    }
    
    return MACHINE_STATE_INIT_GAMEPLAY;
  }

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Continuing with %d balls in trough (%d locked, %d in play)\n", CountBallsInTrough(), NumberOfBallsLocked, NumberOfBallsInPlay);
    Serial.write(buf);
  }
  
  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  RPU_EnableSolenoidStack();
  RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  SamePlayerShootsAgain = false;
  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    RPU_TurnOffAllLamps();
    BallFirstSwitchHitTime = 0;

    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack();
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);
    if (CurrentNumPlayers > 1 && (CurrentBallInPlay != 1 || CurrentPlayer != 0) && !SamePlayerShootsAgain) AlertPlayerUp(CurrentPlayer);
    SamePlayerShootsAgain = false;

    STORM23_SetDisplayBallInPlay(CurrentBallInPlay);
    RPU_SetLampState(LAMP_HEAD_TILT, 0);

    if (BallSaveNumSeconds > 0) {
      RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
    }

    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT;

    for (byte count=0; count<3; count++) {
      DropTargetResetTime[count] = 0;
      DropTargetHurryTime[count] = 0;
      DropTargetHurryLamp[count] = false;
    }
    DropTargetFrenzyEndTime = 0;
    SpinnerFrenzyEndTime = 0;
    LoopMultiballCounter = 0;
    LoopMultiballCountChangedTime = 0;

    ExtraBallCollected = false;
    SpecialCollected = false;

    PlayfieldMultiplier = 1;
    PlayfieldMultiplierExpiration = 0;
    ScoreAdditionAnimation = 0;
    ScoreAdditionAnimationStartTime = 0;
    BonusXAnimationStart = 0;
    LastSpinnerHit = 0;
    LoopSpinnerAlternation = 0;
    LoopSpinnerLastAlternationTime = 0;
    SpinnerLitEndTime = 0;
    BonusChangedTime = 0;
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_BONUS)) Bonus[CurrentPlayer] = 1;
    if (!(HoldoverAwards[CurrentPlayer]&HOLDOVER_BONUS_X)) BonusX[CurrentPlayer] = 1;
    ShowBonusInDisplay();
    Saucer1ScheduledEject = 0;
    Saucer2ScheduledEject = 0;

    BallSaveEndTime = 0;
    IdleMode = IDLE_MODE_NONE;

    LastLeftInlane = 0;
    LastRightInlane = 0;
    SkillShotTarget = 2;

    BonusLastSetTime = 0;
    TimerLastSetTime = 0;
    SpinsLastSetTime = 0;
    LastBonusValueShown = 0;
    LastTimerValueShown = 0;
    LastSpinsValueShown = 0;

    for (byte count = 0; count < NUM_BALL_SEARCH_SOLENOIDS; count++) {
      BallSearchSolenoidFireTime[count] = 0;
    }

    for (byte count=0; count<9; count++) {
      StandupFlashEndTime[count] = 0;
    }

    if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_FRENZY) {
      DropTargetMode[CurrentPlayer] += 1;
      DropTargetStage[CurrentPlayer] = 0;
    }

    // Reset Drop Targets
    DropTargets1.ResetDropTargets(CurrentTime + 100, true);
    DropTargets2.ResetDropTargets(CurrentTime + 250, true);
    DropTargets3.ResetDropTargets(CurrentTime + 400, true);

    RPU_PushToTimedSolenoidStack(SOL_BALL_SERVE, BALL_SERVE_SOLENOID_TIME, CurrentTime + 1000);
    NumberOfBallsInPlay = 1;

    LoopMultiballRunning = false;
    LoopMultiballJackpotReady = false;
    LoopMultiballDoubleJackpotReady = false;
    SpinnerMultiballRunning = false;
    SpinnerMultiballJackpotReady = false;
    SpinnerMultiballDoubleJackpotReady = false;
    StormMultiballRunning = false;
    StormMultiballMegaJackpotReady = false;
    JailBreakMultiballRunning = false;
    JailBreakJackpotReady = false;

    FrenzyDropLastHit = 0;
    LastTurnaroundHit = 0;
    LastSaucer1Hit = 0;
    LastSaucer2Hit = 0;

    // See if any locks have been stolen and move them from locked to availble
    UpdatePlayerLocks();
//    byte rallyNum = CurrentBallInPlay - 1;
//    if (BallsPerGame>3) rallyNum = (CurrentBallInPlay>1) ? ((CurrentBallInPlay==5)?2:1) : 0;
    PlayBackgroundSong(SOUND_EFFECT_RALLY_SONG_1);
  }

  if (CountBallsInTrough()<(TotalBallsLoaded-NumberOfBallsLocked)) {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

  LastTimeThroughLoop = CurrentTime;

  return MACHINE_STATE_INIT_NEW_BALL;
}





void AnnounceStatus() {
  /*
        if (TicksCountedTowardsStatus > 68000) {
          IdleMode = IDLE_MODE_NONE;
          TicksCountedTowardsStatus = 0;
        } else if (TicksCountedTowardsStatus > 59000) {
          if (IdleMode != IDLE_MODE_BALL_SEARCH) {
            BallSearchSolenoidToTry = 0;
            BallSearchNextSolenoidTime = CurrentTime - 1;
          }
          if (CurrentTime > BallSearchNextSolenoidTime) {
            // Fire off a solenoid
            BallSearchSolenoidFireTime[BallSearchSolenoidToTry] = CurrentTime;
            RPU_PushToSolenoidStack(BallSearchSols[BallSearchSolenoidToTry], 10);
            BallSearchSolenoidToTry += 1;
            if (BallSearchSolenoidToTry >= NUM_BALL_SEARCH_SOLENOIDS) BallSearchSolenoidToTry = 0;
            BallSearchNextSolenoidTime = CurrentTime + 500;
          }
          IdleMode = IDLE_MODE_BALL_SEARCH;
        } else if (TicksCountedTowardsStatus > 52000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SHIELD) {
            TicksCountedTowardsStatus = 59001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_SHIELD) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_SHIELD, 1);
            IdleMode = IDLE_MODE_ADVERTISE_SHIELD;
          }
        } else if (TicksCountedTowardsStatus > 45000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS) {
            TicksCountedTowardsStatus = 52001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_SPINS) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_SPINS, 1);
            IdleMode = IDLE_MODE_ADVERTISE_SPINS;
          }
        } else if (TicksCountedTowardsStatus > 38000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_7_NZ) {
            TicksCountedTowardsStatus = 45001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_NZS) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_NZS, 1);
            IdleMode = IDLE_MODE_ADVERTISE_NZS;
            ShowLampAnimation(0, 40, CurrentTime, 11, false, false);
            specialAnimationRunning = true;
          }
        } else if (TicksCountedTowardsStatus > 31000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_POP_BASES) {
            TicksCountedTowardsStatus = 38001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_BASES) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_BASES, 1);
            IdleMode = IDLE_MODE_ADVERTISE_BASES;
          }
        } else if (TicksCountedTowardsStatus > 24000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_COMBOS) {
            TicksCountedTowardsStatus = 31001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_COMBOS) {
              byte countBits = CountBits(CombosAchieved[CurrentPlayer]);
              if (countBits==0) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_COMBOS, 1);
              else if (countBits>0) QueueNotification(SOUND_EFFECT_VP_FIVE_COMBOS_LEFT+(countBits-1), 1);
            }
            IdleMode = IDLE_MODE_ADVERTISE_COMBOS;
          }
        } else if (TicksCountedTowardsStatus > 17000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_INVASION) {
            TicksCountedTowardsStatus = 24001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_INVASION) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_INVASION, 1);
            IdleMode = IDLE_MODE_ADVERTISE_INVASION;
          }
        } else if (TicksCountedTowardsStatus > 10000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_BATTLE) {
            TicksCountedTowardsStatus = 17001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_BATTLE) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_BATTLE, 1);
            IdleMode = IDLE_MODE_ADVERTISE_BATTLE;
          }
        } else if (TicksCountedTowardsStatus > 7000) {
          int goalCount = (int)(CountBits((WizardGoals[CurrentPlayer] & ~UsedWizardGoals[CurrentPlayer]))) + NumCarryWizardGoals[CurrentPlayer];
          if (GoalsUntilWizard==0) {
            TicksCountedTowardsStatus = 10001;
          } else {
            byte goalsRemaining = GoalsUntilWizard-(goalCount%GoalsUntilWizard);
            if (goalCount<0) goalsRemaining = (byte)(-1*goalCount);
            
            if (IdleMode != IDLE_MODE_ANNOUNCE_GOALS) {
              QueueNotification(SOUND_EFFECT_VP_ONE_GOAL_FOR_ENEMY-(goalsRemaining-1), 1);
              if (DEBUG_MESSAGES) {
                char buf[256]; 
                sprintf(buf, "Goals remaining = %d, Goals Until Wiz = %d, goalcount = %d, LO=%d, WizG=0x%04X\n", goalsRemaining, GoalsUntilWizard, goalCount, WizardGoals[CurrentPlayer], NumCarryWizardGoals[CurrentPlayer]);
                Serial.write(buf);
              }
            }
            IdleMode = IDLE_MODE_ANNOUNCE_GOALS;
            ShowLampAnimation(2, 40, CurrentTime, 11, false, false);
            specialAnimationRunning = true;
          }
        }
*/
}


boolean AddABall(boolean ballLocked = false, boolean ballSave = true) {
  if (NumberOfBallsInPlay>=TotalBallsLoaded) return false;

  if (ballLocked) {
    NumberOfBallsLocked += 1;
  } else {
    NumberOfBallsInPlay += 1;
  }

  if (CountBallsInTrough()) {
    RPU_PushToTimedSolenoidStack(SOL_BALL_SERVE, BALL_SERVE_SOLENOID_TIME, CurrentTime + 100);    
  } else {
    if (ballLocked) {
      if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
    } else {
      return false;
    }
  }

  if (ballSave) {
    if (BallSaveEndTime) BallSaveEndTime += 10000;
    else BallSaveEndTime = CurrentTime + 20000;
  }

  return true;
}


void UpdateDropTargets() {
  DropTargets1.Update(CurrentTime);
  DropTargets2.Update(CurrentTime);
  DropTargets3.Update(CurrentTime);

  if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_FRENZY) {
    if (DropTargetFrenzyEndTime && CurrentTime > DropTargetFrenzyEndTime && NumberOfBallsInPlay==1) {
      QueueNotification(SOUND_EFFECT_VP_END_DROP_TARGET_FRENZY, 5);
      DropTargetFrenzyEndTime = 0;
      DropTargetMode[CurrentPlayer] += 1;
      DropTargetStage[CurrentPlayer] = 0;
      if (DropTargets3.GetStatus(false)) DropTargets3.ResetDropTargets(CurrentTime + 250, true);
      if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 500, true);
      if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 750, true);
      DropTargetResetTime[0] = 0;
      DropTargetResetTime[1] = 0;
      DropTargetResetTime[2] = 0;
    } else {
    }
  }


  // Hurry up for drops
  for (byte count=0; count<3; count++) {
    if (DropTargetResetTime[count]) {
      if (CurrentTime>(DropTargetResetTime[count])) {
        DropTargetResetTime[count] = 0;
        if (count==0) DropTargets1.ResetDropTargets(CurrentTime);
        if (count==1) DropTargets2.ResetDropTargets(CurrentTime);
        if (count==2) DropTargets3.ResetDropTargets(CurrentTime);
      } else {
        // Figure out if hurry lamp should be on (and if sound should play)
        if (CurrentTime>DropTargetHurryTime[count]) {
          unsigned long timeUntilNext = (DropTargetResetTime[count] - CurrentTime)/32;
          if (timeUntilNext<50) timeUntilNext = 50;
          DropTargetHurryTime[count] = CurrentTime + timeUntilNext;
          DropTargetHurryLamp[count] ^= 1;
//          if (DropTargetHurryLamp[count]) PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HURRY);
        }
      }
    }    
  }
}

byte GameModeStage;
boolean DisplaysNeedRefreshing = false;
unsigned long LastTimePromptPlayed = 0;
unsigned short CurrentBattleLetterPosition = 0xFF;

#define NUM_GI_FLASH_SEQUENCE_ENTRIES 10
byte GIFlashIndex = 0;
unsigned long GIFlashTimer = 0;
unsigned int GIFlashChangeState[NUM_GI_FLASH_SEQUENCE_ENTRIES] = {1000, 1250, 2000, 2250, 3000, 3300, 3500, 3900, 5000, 6500};

// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  boolean specialAnimationRunning = false;

  UpdateDropTargets();

  // Update timed variables  
  if (SpinnerLitEndTime && CurrentTime>SpinnerLitEndTime) SpinnerLitEndTime = 0;
  if (SpinnerFrenzyEndTime && CurrentTime>SpinnerFrenzyEndTime && NumberOfBallsInPlay==1) {
    // Need to reset music here
    SpinnerFrenzyEndTime = 0;
    QueueNotification(SOUND_EFFECT_VP_END_SPINNER_FRENZY, 5);
  }

  // Figure out if we need to show a timer in the display
  if (PlayfieldMultiplierExpiration || SpinnerFrenzyEndTime || DropTargetFrenzyEndTime || BallSaveEndTime) {
    unsigned long millisecondsLeft = 0;
    if (BallSaveEndTime) {
      millisecondsLeft = BallSaveEndTime - CurrentTime;
    } else if (SpinnerFrenzyEndTime) {
      if (NumberOfBallsInPlay==1) millisecondsLeft = SpinnerFrenzyEndTime - CurrentTime;
    } else if (DropTargetFrenzyEndTime) {
      if (NumberOfBallsInPlay==1) millisecondsLeft = DropTargetFrenzyEndTime - CurrentTime;
    } else {
      if (NumberOfBallsInPlay==1) millisecondsLeft = PlayfieldMultiplierExpiration - CurrentTime;
    }

    if (millisecondsLeft > 99000) millisecondsLeft = 99000;
    if (millisecondsLeft) ShowTimerInDisplay(millisecondsLeft);
  }

  if (LastLeftInlane && CurrentTime>(LastLeftInlane+COMBO_EXPIRATION_TIME)) LastLeftInlane = 0;
  if (LastRightInlane && CurrentTime>(LastRightInlane+COMBO_EXPIRATION_TIME)) LastRightInlane = 0;

  if (ReturnLaneStatus[CurrentPlayer]==0x0F) {
    // They've gotten all the return lanes, so we should light the Bonus X on the loop
    BonusXIncreaseAvailable[CurrentPlayer] += 1;
    if (BonusXIncreaseAvailable[CurrentPlayer]>8) BonusXIncreaseAvailable[CurrentPlayer] = 8;
    ReturnLaneStatus[CurrentPlayer] = 0x00;
  }

  for (byte count=0; count<9; count++) {
    if (StandupFlashEndTime[count] && CurrentTime>StandupFlashEndTime[count]) StandupFlashEndTime[count] = 0;
  }

  if (LoopMultiballCountChangedTime && CurrentTime>(LoopMultiballCountChangedTime+1500)) {
    LoopMultiballCountChangedTime = 0;
  }
  if (NumberOfBallsInPlay==1) {
    // If we drop into single ball, turn off any multiballs running
    if (LoopMultiballRunning || SpinnerMultiballRunning || StormMultiballRunning || JailBreakMultiballRunning) {
      if (StormMultiballRunning) {
        // Drop targets need to be reset at end of storm
        if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 250, true);
        if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 500, true);
        if (DropTargets3.GetStatus(false)) DropTargets3.ResetDropTargets(CurrentTime + 750, true);
        DropTargetResetTime[0] = 0;
        DropTargetResetTime[1] = 0;
        DropTargetResetTime[2] = 0;
      }
      LoopMultiballRunning = false;
      SpinnerMultiballRunning = false;
      StormMultiballRunning = false;
      JailBreakMultiballRunning = false;
      if (DEBUG_MESSAGES) Serial.write("Dropped multiballs because BIP=1\n");
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_AFTER_MULTI);
    }
  } else {
    if (MultiballBallSave==0) BallSaveEndTime = 0;
  }

  if ((CurrentTime - LastSwitchHitTime) > 3000) TimersPaused = true;
  else TimersPaused = false;

  if (Saucer1ScheduledEject && (CurrentTime>Saucer1ScheduledEject || NumberOfBallsInPlay==1)) {
    Saucer1ScheduledEject = 0;
    EjectUnlockedBall(0);
    Audio.StopSound(SOUND_EFFECT_SAUCER_HOLD);
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Eject 0, l=%d, t=%d, bip=%d\n", NumberOfBallsLocked, CountBallsInTrough(), NumberOfBallsInPlay);
      Serial.write(buf);
    }
  }

  if (Saucer2ScheduledEject && (CurrentTime>Saucer2ScheduledEject || NumberOfBallsInPlay==1)) {
    Saucer2ScheduledEject = 0;
    EjectUnlockedBall(1);
    Audio.StopSound(SOUND_EFFECT_SAUCER_HOLD);
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Eject 1, l=%d, t=%d, bip=%d\n", NumberOfBallsLocked, CountBallsInTrough(), NumberOfBallsInPlay);
      Serial.write(buf);
    }
  }

  if (BonusXAnimationStart && CurrentTime>(BonusXAnimationStart+3000)) {
    BonusXAnimationStart = 0;
  }

  if (StormMultiballReady[CurrentPlayer] && CurrentTime>(StormLampsLastChange+380)) {
    StormLampsLastChange = CurrentTime;
    StandupHits[CurrentPlayer] = StandupHits[CurrentPlayer] * 2;
    if (StandupHits[CurrentPlayer] > STANDUPS_ALL) {
      StandupHits[CurrentPlayer] |= STANDUP_1;
      StandupHits[CurrentPlayer] &= STANDUPS_ALL;
    }
  }

  if (FrenzyDropLastHit && CurrentTime>(FrenzyDropLastHit+600)) FrenzyDropLastHit = 0;
  if (LastTurnaroundHit && CurrentTime>(LastTurnaroundHit+600)) LastTurnaroundHit = 0;
  if (LastSaucer1Hit && CurrentTime>(LastSaucer1Hit+600)) LastSaucer1Hit = 0;
  if (LastSaucer2Hit && CurrentTime>(LastSaucer2Hit+600)) LastSaucer2Hit = 0;

  switch ( GameMode ) {
    case GAME_MODE_SKILL_SHOT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        LastTimePromptPlayed = CurrentTime;
        GameModeStage = 0;
      }

      if (BallFirstSwitchHitTime != 0) {
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        if (BallsPerGame==3) {
          PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + (CurrentBallInPlay-1));
        } else {
          byte trackNum = CurrentBallInPlay - 0;
          if (CurrentBallInPlay>1 && CurrentBallInPlay<5) trackNum = 1;
          if (CurrentBallInPlay==5) trackNum = 2;
          PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + trackNum);
        }
      }      
      break;

    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        DisplaysNeedRefreshing = true;
        TicksCountedTowardsStatus = 0;
        IdleMode = IDLE_MODE_NONE;
        if (DEBUG_MESSAGES) {
          Serial.write("Entering unstructured play\n");
        }
      }      

      if (TimersPaused && IdleModeEnabled) {
        TicksCountedTowardsStatus += (CurrentTime - LastTimeThroughLoop);
        AnnounceStatus();
      } else {
        TicksCountedTowardsStatus = 0;
        IdleMode = IDLE_MODE_NONE;
      }
     
      // Playfield X value is only reset during unstructured play
      if (LoopMultiballCountChangedTime) {
        DisplaysNeedRefreshing = true;
        for (byte count = 0; count < 4; count++) {
          OverrideScoreDisplay(count, 1111111*((unsigned long)LoopMultiballCounter), DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
        }
      } else if (PlayfieldMultiplierExpiration) {
        if (CurrentTime > PlayfieldMultiplierExpiration && NumberOfBallsInPlay==1) {
          PlayfieldMultiplierExpiration = 0;
          PlayfieldMultiplier = 1;
        } else {
          ClearDisplayOverride(CurrentPlayer);
          for (byte count = 0; count < 4; count++) {
            if (count != CurrentPlayer) OverrideScoreDisplay(count, PlayfieldMultiplier, DISPLAY_OVERRIDE_ANIMATION_BOUNCE);
          }
          DisplaysNeedRefreshing = true;
        }
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        ShowPlayerScores(0xFF, false, false);
      }

      if (FrenzyDropLastHit) {
        specialAnimationRunning = true;
        ShowLampAnimationSingleStep(2, (LAMP_ANIMATION_STEPS-1) - ((CurrentTime-FrenzyDropLastHit)/25));
      } else if (LastTurnaroundHit) {
        specialAnimationRunning = true;
        ShowLampAnimationSingleStep(0, ((CurrentTime-LastTurnaroundHit)/25)%LAMP_ANIMATION_STEPS);
      } else if (LastSaucer1Hit) {
        specialAnimationRunning = true;
        ShowLampAnimationSingleStep(3, ((CurrentTime-LastSaucer1Hit)/25)%LAMP_ANIMATION_STEPS);
      } else if (LastSaucer2Hit) {
        specialAnimationRunning = true;
        ShowLampAnimationSingleStep(4, ((CurrentTime-LastSaucer2Hit)/25)%LAMP_ANIMATION_STEPS);
      }

      break;
    case GAME_MODE_LOCK_SEQUENCE_1:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 4000;
        GameModeStage = 0;
        if (DEBUG_MESSAGES) {
          Serial.write("Ball 1 lock sequence\n");
        }
      }

      if (GameModeStage==0 && CurrentTime>(GameModeStartTime+2000)) {
        GameModeStage = 1;
        BallFirstSwitchHitTime = 0;
        NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
        if (DEBUG_MESSAGES) {
          Serial.write("Set switch hit time to 0\n");
        }
      }

      if (GameModeStage==1 && CurrentTime>(GameModeStartTime+6000)) {
        GameModeStage = 2;
        QueueNotification(SOUND_EFFECT_VP_CAPTURE_THE_VALKYRIE, 10);
      }      

      if ( (((CurrentTime-GameModeStartTime)/2000)%2)==0 ) {
        specialAnimationRunning = true;
        ShowLampAnimation(4, 41, (CurrentTime-GameModeStartTime), 21, false, false);
      }

      if (GameModeStage && BallFirstSwitchHitTime) {
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Number of balls in play (1) = %d\n", NumberOfBallsInPlay);
          Serial.write(buf);
        }
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);        
      }
      break;
      
    case GAME_MODE_LOCK_SEQUENCE_2:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 4000;        

        GameModeStage = 0;
        if (DEBUG_MESSAGES) {
          Serial.write("Ball 2 lock sequence\n");
        }
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_STORM_READY);
      }

      if ( (((CurrentTime-GameModeStartTime)/3000)%2)==0 ) {
        specialAnimationRunning = true;
        ShowLampAnimation(3, 41, (CurrentTime-GameModeStartTime), 22, false, false);
      }

      if (GameModeStage==0 && CurrentTime>(GameModeStartTime+3000)) {
        GameModeStage = 1;
        BallFirstSwitchHitTime = 0;
        NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "seq2 ML=0x%02X, BL=%d, BIP=%d, T=%d, PL=0x%02X\n", MachineLocks, NumberOfBallsLocked, NumberOfBallsInPlay, CountBallsInTrough(), PlayerLockStatus[CurrentPlayer]);
          Serial.write(buf);
        }
      }

      if (GameModeStage==1 && CurrentTime>(GameModeStartTime+6000)) {
        GameModeStage = 2;
        QueueNotification(SOUND_EFFECT_VP_ODIN_PLEASED, 10);
      }

      if (GameModeStage && BallFirstSwitchHitTime) {
        StormMultiballReady[CurrentPlayer] = true;
        StormLampsLastChange = CurrentTime;
        StandupHits[CurrentPlayer] = STANDUP_1;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);        
      }
      break;

    case GAME_MODE_STORM_ADD_A_BALL:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;        

        // Make sure neither ball kicks
        Saucer1ScheduledEject = 0;
        Saucer2ScheduledEject = 0;

        GameModeStage = 0;
        if (DEBUG_MESSAGES) {
          Serial.write("Storm add a ball sequence\n");
        }
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_MULTIBALL);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(0, 20, (CurrentTime-GameModeStartTime), 20, false, false);

      if (GameModeStage==0 && CurrentTime>(GameModeStartTime+3000)) {
        GameModeStage = 1;
        BallFirstSwitchHitTime = 0;
        NumberOfBallsLocked = 0;
        NumberOfBallsInPlay = 3;
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "addaball ML=%d, BL=%d, BIP=%d, T=%d, PL=%d\n", MachineLocks, NumberOfBallsLocked, NumberOfBallsInPlay, CountBallsInTrough(), PlayerLockStatus[CurrentPlayer]);
          Serial.write(buf);
        }
      }
      if (GameModeStage==1 && CurrentTime>(GameModeStartTime+6000)) {
        GameModeStage = 2;
        QueueNotification(SOUND_EFFECT_VP_RETURN_TO_VALHALLA, 10);
      }

      if (GameModeStage && BallFirstSwitchHitTime) {
        // Kick the saucer balls
        Saucer1ScheduledEject = CurrentTime + 250;
        Saucer2ScheduledEject = CurrentTime + 750;
        MachineLocks = 0;
        NumberOfBallsLocked = 0;
        NumberOfBallsInPlay = 3;
        
        IncreasePlayfieldMultiplier(30000);
        if (MultiballBallSave==0) BallSaveEndTime = 0;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);        
      }
      break;
  }

  if ( !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    ShowBonusXLamps();
    ShowPlayfieldMultiplierLamps();
    ShowStandupLamps();
    ShowLaneAndRolloverLamps();
    ShowLoopAndSpinnerLamps();
    ShowDropTargetLamps();
    ShowShootAgainLamps();
  }

  UpdatePlayfieldDisplay();

  // Three types of display modes are shown here:
  // 1) score animation
  // 2) fly-bys
  // 3) normal scores
  if (ScoreAdditionAnimationStartTime != 0) {
    // Score animation
    if ((CurrentTime - ScoreAdditionAnimationStartTime) < 2000) {
      byte displayPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 60;
      byte digitsToShow = 1 + displayPhase / 6;
      if (digitsToShow > 6) digitsToShow = 6;
      unsigned long scoreToShow = ScoreAdditionAnimation;
      for (byte count = 0; count < (6 - digitsToShow); count++) {
        scoreToShow = scoreToShow / 10;
      }
      if (scoreToShow == 0 || displayPhase % 2) scoreToShow = DISPLAY_OVERRIDE_BLANK_SCORE;
      byte countdownDisplay = (1 + CurrentPlayer) % 4;

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, scoreToShow, DISPLAY_OVERRIDE_ANIMATION_NONE);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, DISPLAY_OVERRIDE_ANIMATION_NONE);
      }
    } else {
      byte countdownDisplay = (1 + CurrentPlayer) % 4;
      unsigned long remainingScore = 0;
      if ( (CurrentTime - ScoreAdditionAnimationStartTime) < 5000 ) {
        remainingScore = (((CurrentTime - ScoreAdditionAnimationStartTime) - 2000) * ScoreAdditionAnimation) / 3000;
        if (PlayScoreAnimationTick>1 && (remainingScore / PlayScoreAnimationTick) != (LastRemainingAnimatedScoreShown / PlayScoreAnimationTick)) {
          LastRemainingAnimatedScoreShown = remainingScore;
          PlaySoundEffect(SOUND_EFFECT_SCORE_TICK);
        }
      } else {
        CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
        remainingScore = 0;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;
      }

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, ScoreAdditionAnimation - remainingScore, DISPLAY_OVERRIDE_ANIMATION_NONE);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, DISPLAY_OVERRIDE_ANIMATION_NONE);
        else OverrideScoreDisplay(count, CurrentScores[CurrentPlayer] + remainingScore, DISPLAY_OVERRIDE_ANIMATION_NONE);
      }
    }
    if (ScoreAdditionAnimationStartTime) ShowPlayerScores(CurrentPlayer, false, false);
    else ShowPlayerScores(0xFF, false, false); 
  } else if (LastSpinnerHit != 0 && TotalSpins[CurrentPlayer]<SpinnerMaxGoal[CurrentPlayer]) {
    OverrideScoreDisplay(CurrentPlayer, SpinnerMaxGoal[CurrentPlayer]-TotalSpins[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_NONE);
    if (CurrentTime>(LastSpinnerHit+3000)) {
      LastSpinnerHit = 0;
      ShowPlayerScores(0xFF, false, false);
    } else {
      ShowPlayerScores(CurrentPlayer, false, false);
    }
  } else {
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - LastTimeScoreChanged) > 2000)) ? true : false);

// this machine doesn't have player up lamps
/*
    // Show the player up lamp
    if (BallFirstSwitchHitTime == 0) {
      for (byte count = 0; count < 4; count++) {
        RPU_SetLampState(LAMP_HEAD_PLAYER_1_UP + count, (((CurrentTime / 250) % 2) == 0 || CurrentPlayer != count) ? false : true);
        RPU_SetLampState(LAMP_HEAD_1_PLAYER + count, ((count+1)==CurrentNumPlayers) ? true : false);
      }
    } else {
      for (byte count = 0; count < 4; count++) {
        RPU_SetLampState(LAMP_HEAD_PLAYER_1_UP + count, (CurrentPlayer == count) ? true : false);
        RPU_SetLampState(LAMP_HEAD_1_PLAYER + count, ((count+1)==CurrentNumPlayers) ? true : false);
      }
    }
*/    
  }

  // Check to see if ball is in the outhole
  if ( /*(NumberOfBallsInPlay+NumberOfBallsLocked)<=TotalBallsLoaded &&*/ CountBallsInTrough()>(TotalBallsLoaded-(NumberOfBallsInPlay+NumberOfBallsLocked))) {

    if (BallTimeInTrough == 0) {
      // If this is the first time we're seeing too many balls in the trough, we'll wait to make sure 
      // everything is settled
      BallTimeInTrough = CurrentTime;
    } else {
      
      // Make sure the ball stays on the sensor for at least
      // 1 second to be sure that it's not bouncing or passing through
      if (CurrentTime > (BallTimeInTrough+1500)) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          if (DEBUG_MESSAGES) {
            char buf[128];
            sprintf(buf, "Kick b/c trough=%d, BIP=%d, Lock=%d\n", CountBallsInTrough(), NumberOfBallsInPlay, NumberOfBallsLocked);
            Serial.write(buf);
          }
          RPU_PushToTimedSolenoidStack(SOL_BALL_SERVE, BALL_SERVE_SOLENOID_TIME, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (BallSaveEndTime && CurrentTime<(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
            RPU_PushToTimedSolenoidStack(SOL_BALL_SERVE, BALL_SERVE_SOLENOID_TIME, CurrentTime + 100);
            QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);
            
            RPU_SetLampState(LAMP_SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

            if (DEBUG_MESSAGES) {
              char buf[255];
              sprintf(buf, "BIT=%d, ML=0x%02X, LS=0x%02X, Numlocks=%d, NumBIP=%d\n", CountBallsInTrough(), MachineLocks, PlayerLockStatus[CurrentPlayer], NumberOfBallsLocked, NumberOfBallsInPlay);
              Serial.write(buf);
            }

            BallSaveEndTime = 0;           
          } else {

            NumberOfBallsInPlay -= 1;
            if (NumberOfBallsInPlay==0) {
              ShowPlayerScores(0xFF, false, false);
              Audio.StopAllAudio();
              returnState = MACHINE_STATE_COUNTDOWN_BONUS;
            }
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  LastTimeThroughLoop = CurrentTime;
  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;
byte DecrementingBonusCounter;
byte IncrementingBonusXCounter;
byte TotalBonus = 0;
byte TotalBonusX = 0;
boolean CountdownBonusHurryUp = false;

int CountDownDelayTimes[] = {175, 130, 105, 90, 80, 70, 60, 40, 30, 20};

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {

    CountdownStartTime = CurrentTime;
    LastCountdownReportTime = CurrentTime;
    RPU_TurnOffAllLamps();
    BonusXAnimationStart = CurrentTime;
    ShowBonusXLamps();
    ShowBonusInDisplay();
    IncrementingBonusXCounter = 1;
    DecrementingBonusCounter = Bonus[CurrentPlayer];
    TotalBonus = Bonus[CurrentPlayer];
    TotalBonusX = BonusX[CurrentPlayer];
    CountdownBonusHurryUp = false;

    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  unsigned long countdownDelayTime = (unsigned long)(CountDownDelayTimes[IncrementingBonusXCounter-1]);
  if (CountdownBonusHurryUp && countdownDelayTime>((unsigned long)CountDownDelayTimes[9])) countdownDelayTime=CountDownDelayTimes[9];
  
  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (DecrementingBonusCounter) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        PlaySoundEffect(SOUND_EFFECT_BONUS_COLLECT);
        CurrentScores[CurrentPlayer] += 1000;        
      }

      DecrementingBonusCounter -= 1;
      Bonus[CurrentPlayer] = DecrementingBonusCounter;
      if (Bonus[CurrentPlayer]) ShowBonusInDisplay();
      else STORM23_SetPlayfieldDisplay(0, false, false, false);
      UpdatePlayfieldDisplay();

    } else if (BonusCountDownEndTime==0xFFFFFFFF) {
      IncrementingBonusXCounter += 1;
      if (BonusX[CurrentPlayer]>1) {
        DecrementingBonusCounter = TotalBonus;
        Bonus[CurrentPlayer] = TotalBonus;
        ShowBonusInDisplay();
        BonusX[CurrentPlayer] -= 1;
        if (BonusX[CurrentPlayer]==9) BonusX[CurrentPlayer] = 8;
        BonusXAnimationStart = CurrentTime;
        ShowBonusXLamps();
      } else {
        BonusX[CurrentPlayer] = TotalBonusX;
        Bonus[CurrentPlayer] = TotalBonus;
        BonusCountDownEndTime = CurrentTime + 1000;
      }
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_TIME, CurrentTime, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_TIME, CurrentTime + 300, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_TIME, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
//    RPU_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      STORM23_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
//      RPU_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
//        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
//        RPU_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      byte upperMask = 0x0F;
      byte lowerMask = 0x30;
      if (RPU_OS_NUM_DIGITS==7) {
        upperMask = 0x1F;
        lowerMask = 0x60;      
      }
      if ( (CurrentTime / 200) % 2 ) {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & upperMask);
      } else {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | lowerMask);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}




////////////////////////////////////////////////////////////////////////////
//
//  Switch Handling functions
//
////////////////////////////////////////////////////////////////////////////
void HandleSaucerUpSwitch(byte switchIndex) {

  if (MachineState==MACHINE_STATE_NORMAL_GAMEPLAY) {
    // see if this up switch is unexpected
    byte saucerMask = LOCK_1_ENGAGED<<switchIndex;

    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Saw up on %d, ML=0x%02X, SM=0x%02X\n", switchIndex, MachineLocks, saucerMask);
      Serial.write(buf);
    }
    
    if (MachineLocks & saucerMask) {
      // We're supposed to have a lock on the ball in this saucer, so this is unexpected
      StartJailBreakMultiball();
    }
  }
    
}


void HandleSaucerDownSwitch(byte switchIndex) {

  // This switch is only "new" if it's not reflected in MachineLocks
  if (MachineLocks & (LOCK_1_ENGAGED<<switchIndex)) {
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Ignoring lock switch %d because ML=0x%04X\n", switchIndex, MachineLocks);
      Serial.write(buf);
    }
    return;
  }

  if (MachineState==MACHINE_STATE_NORMAL_GAMEPLAY) {

    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "handling lock index %d (bip=%d, pl=0x%02X, ml=0x%02X)\n", switchIndex, NumberOfBallsInPlay, PlayerLockStatus[CurrentPlayer], MachineLocks);
      Serial.write(buf);
    }

    if (NumberOfBallsInPlay<2 && PlayerLockStatus[CurrentPlayer] & (LOCK_1_AVAILABLE<<switchIndex)) {
      // Our lock is available, so we should lock
      LockBall(switchIndex, false);
      QueueNotification(SOUND_EFFECT_VP_BALL_LOCKED, 10);
      if (CountBits(PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK)==1) SetGameMode(GAME_MODE_LOCK_SEQUENCE_1);
      else SetGameMode(GAME_MODE_LOCK_SEQUENCE_2);

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "lock ML=0x%02X, BL=%d, BIP=%d, T=%d, PL=0x%02X\n", MachineLocks, NumberOfBallsLocked, NumberOfBallsInPlay, CountBallsInTrough(), PlayerLockStatus[CurrentPlayer]);
        Serial.write(buf);
      }
      
    } else if (StormMultiballRunning && NumberOfBallsInPlay>1) {
      StormMultiballMegaJackpotReady = true;
      
      // Storm multiball handler
      // Figure out if there are one or two balls in saucers
      byte numBallsInSaucers = 1;
      if (switchIndex==0 && UpperLockSwitchState[1]) numBallsInSaucers += 1;
      if (switchIndex==1 && UpperLockSwitchState[2]) numBallsInSaucers += 1;

      if (numBallsInSaucers==2) {
        if (NumberOfBallsInPlay==2) {
          // Add a ball
          SetGameMode(GAME_MODE_STORM_ADD_A_BALL);
          StartScoreAnimation( PlayfieldMultiplier * 2 * STORM_MULTIBALL_JACKPOT_VALUE );
          QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 10);
        } else {
          // Score a double Jackpot
          if (switchIndex==0) {
            Saucer1ScheduledEject = CurrentTime + 15000;
            Saucer2ScheduledEject = CurrentTime + 3000;
          } else if (switchIndex==1) {
            Saucer2ScheduledEject = CurrentTime + 15000;
            Saucer1ScheduledEject = CurrentTime + 3000;
          }
          StartScoreAnimation( PlayfieldMultiplier * 5 * STORM_MULTIBALL_JACKPOT_VALUE );
          QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 10);
        }          
      } else {
        // This is a regular jackpot
        if (switchIndex==0) Saucer1ScheduledEject = CurrentTime + 15000;
        else if (switchIndex==1) Saucer2ScheduledEject = CurrentTime + 15000;
        PlaySoundEffect(SOUND_EFFECT_SAUCER_HOLD);
        StartScoreAnimation( PlayfieldMultiplier * 2 * STORM_MULTIBALL_JACKPOT_VALUE );
        QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 10);
      }
    } else if (LoopMultiballRunning && switchIndex==0 && LoopMultiballDoubleJackpotReady) {
      // During loop multiball, the top saucer can award a jackpot
      LoopMultiballDoubleJackpotReady = false;
      Saucer1ScheduledEject = CurrentTime + 10000;
      StartScoreAnimation( ((unsigned long)LoopMulitballGoal[CurrentPlayer]) * PlayfieldMultiplier * 2 * LOOP_MULTIBALL_JACKPOT_VALUE );
      QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 10);
    } else if (SpinnerMultiballRunning && switchIndex==1 && SpinnerMultiballDoubleJackpotReady) {
      // During spinner multiball, the right saucer can award a jackpot
      SpinnerMultiballDoubleJackpotReady = false;
      Saucer2ScheduledEject = CurrentTime + 10000;
      StartScoreAnimation( PlayfieldMultiplier * 2 * SPINNER_MULTIBALL_JACKPOT_VALUE );
      QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 10);
    } else {
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
      PlaySoundEffect(SOUND_EFFECT_SAUCER_REJECTED);
      // Have to request eject before spotting standup target
      // or else the eject will fail based on locks
      if (DEBUG_MESSAGES) {
        Serial.write("default = eject and spot (when applicable)\n");
      }
      EjectUnlockedBall(switchIndex);
      if (NumberOfBallsInPlay<2) SpotStandupTarget(switchIndex);
      LastSaucer1Hit = CurrentTime;
    }
  } else {
    if (DEBUG_MESSAGES) {
      Serial.write("Handling saucer outside of game play\n");
    }  
  }
}


boolean AwardCombo(byte comboFlag) {

  if ( (CombosAchieved[CurrentPlayer]&comboFlag)==0 ) {
    CombosAchieved[CurrentPlayer] |= comboFlag;
    byte numCombos = CountBits(CombosAchieved[CurrentPlayer]);
    StartScoreAnimation(10000 * ((unsigned long)numCombos) * PlayfieldMultiplier);
    QueueNotification(SOUND_EFFECT_VP_COMBO_1 + (numCombos-1), 5);

    if (comboFlag & (COMBO_LEFT_TO_2BANK | COMBO_LEFT_TO_LOOP | COMBO_LEFT_TO_SPINNER | COMBO_LEFT_TO_RIGHT)) {
      LastLeftInlane = 0;
    } else {
      LastRightInlane = 0;
    }

    if (numCombos==3) {
      HoldoverAwards[CurrentPlayer] |= HOLDOVER_BONUS_X;
      QueueNotification(SOUND_EFFECT_VP_BONUS_X_HELD, 5);
    } else if (numCombos==6) {
      HoldoverAwards[CurrentPlayer] |= HOLDOVER_BONUS;      
      QueueNotification(SOUND_EFFECT_VP_BONUS_HELD, 5);
    } else if (numCombos==8) {
      FeaturesCompleted[CurrentPlayer] |= FEATURE_ALL_COMBOS;
      HoldoverAwards[CurrentPlayer] |= HOLDOVER_TIMER_MULTIPLIER;
      QueueNotification(SOUND_EFFECT_VP_TIMERS_DOUBLED, 5);
    }
    
    return true;    
  }

  // Player has already gotten this combo
  return false;  
}


int HandleSystemSwitches(int curState, byte switchHit) {
  int returnState = curState;
  switch (switchHit) {
    case SW_SELF_TEST_SWITCH:
      returnState = MACHINE_STATE_TEST_LAMPS;
      SetLastSelfTestChangedTime(CurrentTime);
      break;
    case SW_COIN_1:
    case SW_COIN_2:
    case SW_COIN_3:
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
      break;
    case SW_CREDIT_RESET:
      if (MachineState == MACHINE_STATE_MATCH_MODE) {
        // If the first ball is over, pressing start again resets the game
        if (Credits >= 1 || FreePlayMode) {
          if (!FreePlayMode) {
            Credits -= 1;
            RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
            STORM23_SetDisplayCredits(Credits, !FreePlayMode);
          }
          returnState = MACHINE_STATE_INIT_GAMEPLAY;
        }
      } else {
        CreditResetPressStarted = CurrentTime;
      }
      break;
    case SW_OUTHOLE:
      MoveBallFromOutholeToRamp(true);
      break;
    case SW_PLUMB_TILT:
//    case SW_ROLL_TILT:
//    case SW_PLAYFIELD_TILT:
      // This should be debounced
      if (IdleMode != IDLE_MODE_BALL_SEARCH && (CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
        LastTiltWarningTime = CurrentTime;
        NumTiltWarnings += 1;
        if (NumTiltWarnings > MaxTiltWarnings) {
          RPU_DisableSolenoidStack();
          RPU_SetDisableFlippers(true);
          RPU_TurnOffAllLamps();
          RPU_SetLampState(LAMP_HEAD_TILT, 1);
          Audio.StopAllAudio();
        }
        PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
      }
      break;  
  }

  return returnState;
}


boolean HandleDropTarget(byte bankNum, byte switchHit) {

  DropTargetBank *curBank;
  if (bankNum==0) curBank = &DropTargets1;
  else if (bankNum==1) curBank = &DropTargets2;
  else if (bankNum==2) curBank = &DropTargets3;

  byte result;
  unsigned long numTargetsDown = 0;
  result = curBank->HandleDropTargetHit(switchHit);
  numTargetsDown = (unsigned long)CountBits(result);

  if (numTargetsDown==0) return false; // dupe hit -- ignore

  if (StormMultiballRunning) {
    AddToBonus(numTargetsDown);
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    boolean cleared = curBank->CheckIfBankCleared();
    if (cleared) {
      if (BonusXIncreaseAvailable[CurrentPlayer]<8) BonusXIncreaseAvailable[CurrentPlayer] += 1;      
      // Check to see if all are cleared
      if (DropTargets1.CheckIfBankCleared() && DropTargets1.CheckIfBankCleared() && DropTargets1.CheckIfBankCleared()) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2 * DROP_TARGET_JACKPOT_VALUE;      
        QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 6);
        FeaturesCompleted[CurrentPlayer] |= FEATURE_JACKPOT_AWARDED;
        DropTargetResetTime[0] = 2000 + CurrentTime;
        DropTargetResetTime[1] = 4000 + CurrentTime;
        DropTargetResetTime[2] = 6000 + CurrentTime;
      } else {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;      
        PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_STORM_CLEARED);
      }
      
    } else {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_STORM_HIT);
    }
    
  } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_IN_ORDER) {
    AddToBonus(numTargetsDown);
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;

    boolean cleared = curBank->CheckIfBankCleared();
    if (cleared) {
      if ((LoopMultiballRunning||SpinnerMultiballRunning) && BonusXIncreaseAvailable[CurrentPlayer]<8) BonusXIncreaseAvailable[CurrentPlayer] += 1;      
      if (bankNum==DropTargetStage[CurrentPlayer]) {
        if (bankNum<2) {
          DropTargetStage[CurrentPlayer] += 1;
          curBank->ResetDropTargets(CurrentTime + 500, true);
          DropTargetResetTime[bankNum] = 0;
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_STAGE_COMPLETE);
        } else {
          FeaturesCompleted[CurrentPlayer] |= FEATURE_DROP_TARGET_FRENZY;
          DropTargetStage[CurrentPlayer] = 0;
          DropTargetMode[CurrentPlayer] = DROP_TARGETS_FRENZY;
          IncreasePlayfieldMultiplier(DROP_TARGET_FRENZY_TIME);
          unsigned long timerMultiplier = 1;
          if (HoldoverAwards[CurrentPlayer]&HOLDOVER_TIMER_MULTIPLIER) timerMultiplier = 2;
          if (SpinnerFrenzyEndTime) {
            DropTargetFrenzyEndTime = SpinnerFrenzyEndTime + DROP_TARGET_FRENZY_TIME * timerMultiplier;
            SpinnerFrenzyEndTime = DropTargetFrenzyEndTime;
          } else {
            DropTargetFrenzyEndTime = CurrentTime + DROP_TARGET_FRENZY_TIME * timerMultiplier;
          }
          DropTargets3.ResetDropTargets(CurrentTime + 250, true);
          if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 500, true);
          if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 750, true);
          DropTargetResetTime[0] = 0;
          DropTargetResetTime[1] = 0;
          DropTargetResetTime[2] = 0;
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_MODE_COMPLETE);
          QueueNotification(SOUND_EFFECT_VP_DROP_TARGET_FRENZY, 8);
        }
      } else {
        curBank->ResetDropTargets(CurrentTime + 500, true);
        DropTargetResetTime[bankNum] = 0;
        PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_RESET);
      }
    } else {
      PlaySoundEffect(SOUND_EFFECT_LIGHTNING_1 + CurrentTime%12);
    }

  } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_FRENZY) {
    AddToBonus(numTargetsDown);
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2500;
    boolean cleared = curBank->CheckIfBankCleared();
    if (cleared) {
      if ((LoopMultiballRunning||SpinnerMultiballRunning) && BonusXIncreaseAvailable[CurrentPlayer]<8) BonusXIncreaseAvailable[CurrentPlayer] += 1;      
      curBank->ResetDropTargets(CurrentTime + 500, true);
      DropTargetResetTime[bankNum] = 0;
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_RESET);
    } else {
      if (DropTargetResetTime[bankNum]==0) DropTargetResetTime[bankNum] = 3000 + CurrentTime;
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_VALUE_HIT);
      FrenzyDropLastHit = CurrentTime;
    }
  } else if (DropTargetMode[CurrentPlayer]>=DROP_TARGETS_JACKPOT) {

    if (bankNum==DropTargetStage[CurrentPlayer]) {
      AddToBonus(numTargetsDown);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;

      boolean cleared = curBank->CheckIfBankCleared();
      if (cleared) {
        if ((LoopMultiballRunning||SpinnerMultiballRunning) && BonusXIncreaseAvailable[CurrentPlayer]<8) BonusXIncreaseAvailable[CurrentPlayer] += 1;      
        
        if (DropTargetStage[CurrentPlayer]<2) {
          DropTargetStage[CurrentPlayer] += 1;
          DropTargetResetTime[bankNum] = 0;
        } else {
          DropTargetStage[CurrentPlayer] = 0;
          if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_JACKPOT) {
            QueueNotification(SOUND_EFFECT_VP_JACKPOT, 10);
            FeaturesCompleted[CurrentPlayer] |= FEATURE_JACKPOT_AWARDED;
            StartScoreAnimation(PlayfieldMultiplier * DROP_TARGET_JACKPOT_VALUE);
            DropTargetMode[CurrentPlayer] += 1;
          } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_DOUBLE_JACKPOT) {
            QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 10);
            StartScoreAnimation(PlayfieldMultiplier * 2 * DROP_TARGET_JACKPOT_VALUE);
            DropTargetMode[CurrentPlayer] += 1;
          } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_SUPER_JACKPOT) {
            QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 10);
            StartScoreAnimation(PlayfieldMultiplier * 5 * DROP_TARGET_JACKPOT_VALUE);
            DropTargetMode[CurrentPlayer] += 1;
          } else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_MEGA_JACKPOT) {
            QueueNotification(SOUND_EFFECT_VP_MEGA_JACKPOT, 10);
            StartScoreAnimation(PlayfieldMultiplier * 10 * DROP_TARGET_JACKPOT_VALUE);
            DropTargetMode[CurrentPlayer] = 0;
          }

          if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 250, true);
          if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 500, true);
          if (DropTargets3.GetStatus(false)) DropTargets3.ResetDropTargets(CurrentTime + 750, true);
          DropTargetResetTime[0] = 0;
          DropTargetResetTime[1] = 0;
          DropTargetResetTime[2] = 0;
        }
        
      } else {
        PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);

        if (DropTargetMode[CurrentPlayer]>DROP_TARGETS_JACKPOT) {
          unsigned long resetTime = 20000;
          if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_SUPER_JACKPOT) resetTime = 15000;
          else if (DropTargetMode[CurrentPlayer]==DROP_TARGETS_MEGA_JACKPOT) resetTime = 10000;

          DropTargetResetTime[bankNum] = CurrentTime + resetTime;
        }
      }
      
    } else {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_NO_POINTS);
      if (bankNum>DropTargetStage[CurrentPlayer]) {
        // Reset drops and stage
        if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 1250, true);
        if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 1500, true);
        if (DropTargets3.GetStatus(false)) DropTargets3.ResetDropTargets(CurrentTime + 1750, true);
        DropTargetResetTime[0] = 0;
        DropTargetResetTime[1] = 0;
        DropTargetResetTime[2] = 0;
        DropTargetStage[CurrentPlayer] = 0;
      }
    }

    boolean cleared = curBank->CheckIfBankCleared();
    if (cleared) {
      if (bankNum==DropTargetStage[CurrentPlayer]) {
        if (bankNum<2) {
          DropTargetStage[CurrentPlayer] += 1;
          curBank->ResetDropTargets(CurrentTime + 500, true);
          DropTargetResetTime[bankNum] = 0;
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_STAGE_COMPLETE);
        } else {
          DropTargetStage[CurrentPlayer] = 0;
          DropTargetMode[CurrentPlayer] = DROP_TARGETS_FRENZY;
          IncreasePlayfieldMultiplier(DROP_TARGET_FRENZY_TIME);
          unsigned long timerMultiplier = 1;
          if (HoldoverAwards[CurrentPlayer]&HOLDOVER_TIMER_MULTIPLIER) timerMultiplier = 2;
          if (SpinnerFrenzyEndTime) {
            DropTargetFrenzyEndTime = SpinnerFrenzyEndTime + DROP_TARGET_FRENZY_TIME * timerMultiplier;
          } else {
            DropTargetFrenzyEndTime = CurrentTime + DROP_TARGET_FRENZY_TIME * timerMultiplier;
          }
          DropTargets3.ResetDropTargets(CurrentTime + 250, true);
          if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 500, true);
          if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 750, true);
          DropTargetResetTime[0] = 0;
          DropTargetResetTime[1] = 0;
          DropTargetResetTime[2] = 0;
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_MODE_COMPLETE);
        }
      } else {
        curBank->ResetDropTargets(CurrentTime + 500, true);
        DropTargetResetTime[bankNum] = 0;
        PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_RESET);
      }
    } else {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
    }  
  }
  
  return true;
}


void HandleSpinnerMultiball() {
  if (PlayerLockStatus[CurrentPlayer] & LOCK_2_ENGAGED) {
    LastSpinnerHit = CurrentTime;
    TotalSpins[CurrentPlayer] += 1;
    ShowSpinsInDisplay(SpinnerMaxGoal[CurrentPlayer] - TotalSpins[CurrentPlayer]);

    if (TotalSpins[CurrentPlayer]==SpinnerMaxGoal[CurrentPlayer]) {
      FeaturesCompleted[CurrentPlayer] |= FEATURE_SPINNER_MULTIBALL;
      LastSpinnerHit = 0;
      TotalSpins[CurrentPlayer] = 0;
      SpinnerMultiballDoubleJackpotReady = false;
      if (SpinnerMaxGoal[CurrentPlayer]<226) SpinnerMaxGoal[CurrentPlayer] += 25; 
      QueueNotification(SOUND_EFFECT_VP_SPINNER_MULTIBALL_START, 10);
      
      IncreasePlayfieldMultiplier(15000);
      IncreasePlayfieldMultiplier(15000);
      ReleaseLockedBall(1);

      // Relinquish locked standups
      StandupHits[CurrentPlayer] &= ~STANDUPS_FOR_LOCK_2;

      SpinnerMultiballRunning = true;
      if (MultiballBallSave) {
        BallSaveEndTime = CurrentTime + ((unsigned long)BallSaveNumSeconds)*1000;
      }
      SpinnerMultiballJackpotReady = true;
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_MULTIBALL);
    }
    
  }
}


void HandleLoopMultiball() {
  if (PlayerLockStatus[CurrentPlayer] & LOCK_1_ENGAGED) {
    LoopMultiballCounter += 1;
    LoopMultiballCountChangedTime = CurrentTime;

    if (LoopMultiballCounter==LoopMulitballGoal[CurrentPlayer]) {
      FeaturesCompleted[CurrentPlayer] |= FEATURE_LOOP_MULTIBALL;
      LoopMulitballGoal[CurrentPlayer] += 1;
      if (LoopMulitballGoal[CurrentPlayer]>9) LoopMulitballGoal[CurrentPlayer] = 9;

      QueueNotification(SOUND_EFFECT_VP_LOOP_MULTIBALL_START, 10);

      IncreasePlayfieldMultiplier(15000);
      IncreasePlayfieldMultiplier(15000);
      ReleaseLockedBall(0);
      // Relinquish locked standups
      StandupHits[CurrentPlayer] &= ~STANDUPS_FOR_LOCK_1;
      
      LoopMultiballRunning = true;
      if (MultiballBallSave) {
        BallSaveEndTime = CurrentTime + ((unsigned long)BallSaveNumSeconds)*1000;
      }
      LoopMultiballJackpotReady = false;
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_MULTIBALL);
    }
  } else if (LoopMultiballRunning) {
    if (LoopMultiballJackpotReady) {
      LoopMultiballJackpotReady = false;
      if (Saucer1ScheduledEject) {
        QueueNotification(SOUND_EFFECT_VP_MEGA_JACKPOT, 10);
        StartScoreAnimation( ((unsigned long)LoopMulitballGoal[CurrentPlayer]) * PlayfieldMultiplier * 10 * LOOP_AND_SPINNER_JACKPOT_VALUE );
      } else {
        QueueNotification(SOUND_EFFECT_VP_JACKPOT, 9);
        FeaturesCompleted[CurrentPlayer] |= FEATURE_JACKPOT_AWARDED;
        StartScoreAnimation( ((unsigned long)LoopMulitballGoal[CurrentPlayer]) * PlayfieldMultiplier * LOOP_AND_SPINNER_JACKPOT_VALUE );
      }
    } 
  }
}


void HandleLoopSpinnerAlternation(byte switchHit) {

  byte numAlternationsNeeded = 6 + AlternationMode[CurrentPlayer]*2;

  if (SpinnerFrenzyEndTime) return;

  if ( (LoopSpinnerAlternation%2)==0 ) {
    // only way to advance is to hit the loop
    if (switchHit==SW_LOOP) {
      LoopSpinnerAlternation += 1;
      PlaySoundEffect(SOUND_EFFECT_ALTERNATION_ADVANCE);
    }
  } else {
    // only way to advance is to hit the spinner
    if (switchHit==SW_SPINNER) {
      LoopSpinnerAlternation += 1;
      PlaySoundEffect(SOUND_EFFECT_ALTERNATION_ADVANCE);
      SpinnerLitEndTime = CurrentTime + 5000;
    }
  }

  if (LoopSpinnerAlternation==numAlternationsNeeded) {
    FeaturesCompleted[CurrentPlayer] |= FEATURE_SPINNER_FRENZY;
    LoopSpinnerAlternation = 0;
    QueueNotification(SOUND_EFFECT_VP_SPINNER_FRENZY, 10);
    
    if (DropTargetFrenzyEndTime) {
      SpinnerFrenzyEndTime = DropTargetFrenzyEndTime + SPINNER_FRENZY_TIME;
      DropTargetFrenzyEndTime = SpinnerFrenzyEndTime;
    } else {
      SpinnerFrenzyEndTime = CurrentTime + SPINNER_FRENZY_TIME;
    }

    IncreasePlayfieldMultiplier(SPINNER_FRENZY_TIME);
  }
}


void UpdateLocksBasedOnStandups(unsigned int startingHits) {

  boolean multiballRunning = LoopMultiballRunning || SpinnerMultiballRunning || StormMultiballRunning || JailBreakMultiballRunning; 

  // Check to see if either lock is ready or engaged now
  if ( (startingHits&STANDUPS_FOR_LOCK_1)!=STANDUPS_FOR_LOCK_1 && (StandupHits[CurrentPlayer]&STANDUPS_FOR_LOCK_1)==STANDUPS_FOR_LOCK_1 ) {
    // Lock one is now ready/engaged
    if ( !(PlayerLockStatus[CurrentPlayer]&LOCK_1_ENGAGED) && !(PlayerLockStatus[CurrentPlayer]&LOCK_1_AVAILABLE) ) {
      // This should always be the case -- but who knows?
      if (MachineLocks&LOCK_1_ENGAGED) {
        // Ball is already there, so announce lock
        if (!multiballRunning) QueueNotification(SOUND_EFFECT_VP_BALL_1_LOCKED, 10);
        LockBall(0);
      } else {
        // annouce lock ready
        if (!multiballRunning) QueueNotification(SOUND_EFFECT_VP_LOCK_1_READY, 10);
        PlayerLockStatus[CurrentPlayer] |= LOCK_1_AVAILABLE;
      }
    }

    for (byte count=2; count<7; count++) {
      StandupFlashEndTime[count] = CurrentTime + 5000;
    }
    
  }

  if ( (startingHits&STANDUPS_FOR_LOCK_2)!=STANDUPS_FOR_LOCK_2 && (StandupHits[CurrentPlayer]&STANDUPS_FOR_LOCK_2)==STANDUPS_FOR_LOCK_2 ) {
    // Lock two is now ready/engaged
    if ( !(PlayerLockStatus[CurrentPlayer]&LOCK_2_ENGAGED) && !(PlayerLockStatus[CurrentPlayer]&LOCK_2_AVAILABLE) ) {
      // This should always be the case -- but who knows?
      if (MachineLocks&LOCK_2_ENGAGED) {
        // Ball is already there, so announce lock
        if (!multiballRunning) QueueNotification(SOUND_EFFECT_VP_BALL_2_LOCKED, 10);
        LockBall(1);
      } else {
        // annouce lock ready
        if (!multiballRunning) QueueNotification(SOUND_EFFECT_VP_LOCK_2_READY, 10);
        PlayerLockStatus[CurrentPlayer] |= LOCK_2_AVAILABLE;
      }
      TotalSpins[CurrentPlayer] = 0;        
    }    
    for (byte count=0; count<2; count++) {
      StandupFlashEndTime[count] = CurrentTime + 5000;
    }
    for (byte count=7; count<9; count++) {
      StandupFlashEndTime[count] = CurrentTime + 5000;
    }
  }
  
}


void SpotStandupTarget(byte targetBank) {
  if (FeaturesCompleted[CurrentPlayer] & FEATURE_STORM_MULTIBALL) return;
  if ((FeaturesCompleted[CurrentPlayer] & FEATURE_LOOP_MULTIBALL) && targetBank==0) return;
  if ((FeaturesCompleted[CurrentPlayer] & FEATURE_SPINNER_MULTIBALL) && targetBank==1) return;
  if (targetBank==0 && (StandupHits[CurrentPlayer]&STANDUPS_FOR_LOCK_1)==STANDUPS_FOR_LOCK_1) return;
  if (targetBank==1 && (StandupHits[CurrentPlayer]&STANDUPS_FOR_LOCK_2)==STANDUPS_FOR_LOCK_2) return;

  unsigned short startingHits = StandupHits[CurrentPlayer];
  unsigned int targetsNeeded = STANDUPS_FOR_LOCK_1;
  if (targetBank==1) targetsNeeded = STANDUPS_FOR_LOCK_2;
  unsigned int curTargets = StandupHits[CurrentPlayer];

  byte spotIndex = 0xFF;
  for (byte count=0; count<9; count++) {
    if ( (targetsNeeded & 0x01) && (curTargets & 0x01)==0) {
      spotIndex = count;
      break;
    }

    targetsNeeded /= 2;
    curTargets /= 2;
  }

  if (spotIndex!=0xFF) {
    StandupHits[CurrentPlayer] |= (0x01<<spotIndex);
    StandupFlashEndTime[spotIndex] = CurrentTime + 5000;
    
    if (targetBank==0 && (StandupHits[CurrentPlayer]&STANDUPS_FOR_LOCK_1)==STANDUPS_FOR_LOCK_1) {
      for (byte count=2; count<7; count++) {
        StandupFlashEndTime[count] = CurrentTime + 5000;
      }
    }
    if (targetBank==1 && (StandupHits[CurrentPlayer]&STANDUPS_FOR_LOCK_2)==STANDUPS_FOR_LOCK_2) {
      for (byte count=0; count<2; count++) {
        StandupFlashEndTime[count] = CurrentTime + 5000;
      }
      for (byte count=7; count<9; count++) {
        StandupFlashEndTime[count] = CurrentTime + 5000;
      }
    }

    UpdateLocksBasedOnStandups(startingHits);
  }
}


void HandleStandupTarget(byte switchHit) {
  unsigned short startingHits = StandupHits[CurrentPlayer];
  byte targetIndex = SW_TARGET_1 - switchHit;
  if (switchHit==SW_TARGET_9) targetIndex = 8;
  unsigned int targetBit = STANDUP_1 << targetIndex;

  unsigned long targetAlreadyFlashingMultiplier = (StandupFlashEndTime[targetIndex]) ? 2 : 1;

  StandupFlashEndTime[targetIndex] = CurrentTime + 2000;

  if (GameMode==GAME_MODE_SKILL_SHOT) {
    if (SkillShotTarget==targetIndex) {
      QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT, 6);
      StartScoreAnimation(PlayfieldMultiplier * ((unsigned long)CurrentBallInPlay) * 25000);

      for (byte count=2; count<7; count++) {
        StandupFlashEndTime[count] = CurrentTime + 4000;
      }
    }
  }

  if (StandupHits[CurrentPlayer]&targetBit) {
    // Already hit this one
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000 * targetAlreadyFlashingMultiplier;
    PlaySoundEffect(SOUND_EFFECT_STANDUP_LIT);

    // If we hit a lit target w/storm ready, start storm
    if (StormMultiballReady[CurrentPlayer]) {
      FeaturesCompleted[CurrentPlayer] |= FEATURE_STORM_MULTIBALL;
      QueueNotification(SOUND_EFFECT_VP_STORM_MULTIBALL_START, 10);

      IncreasePlayfieldMultiplier(15000);
      IncreasePlayfieldMultiplier(15000);
      IncreasePlayfieldMultiplier(15000);
      ReleaseLockedBall(0);
      ReleaseLockedBall(1);
      // Relinquish locked standups
      StandupHits[CurrentPlayer] &= ~STANDUPS_FOR_LOCK_1;
      StandupHits[CurrentPlayer] &= ~STANDUPS_FOR_LOCK_2;

      // Reset drops
      if (DropTargets1.GetStatus(false)) DropTargets1.ResetDropTargets(CurrentTime + 250, true);
      if (DropTargets2.GetStatus(false)) DropTargets2.ResetDropTargets(CurrentTime + 500, true);
      if (DropTargets3.GetStatus(false)) DropTargets3.ResetDropTargets(CurrentTime + 750, true);

      StormMultiballReady[CurrentPlayer] = false;
      StormMultiballRunning = true;
      if (MultiballBallSave) {
        BallSaveEndTime = CurrentTime + ((unsigned long)BallSaveNumSeconds)*1000;
      }
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_MULTIBALL);
    }
  } else {
    // Hitting target for first time
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100 * targetAlreadyFlashingMultiplier;
    PlaySoundEffect(SOUND_EFFECT_STANDUP);
    StandupHits[CurrentPlayer] |= targetBit;

    UpdateLocksBasedOnStandups(startingHits);
  }
  
}


void HandleGamePlaySwitches(byte switchHit) {

  if (MachineState!=MACHINE_STATE_NORMAL_GAMEPLAY) return;

  switch (switchHit) {

    case SW_LEFT_SLING_BOTTOM:
    case SW_RIGHT_SLING_BOTTOM:
      if (CurrentTime < (BallSearchSolenoidFireTime[BALL_SEARCH_LEFT_SLING_INDEX] + 150)) break;
      if (CurrentTime < (BallSearchSolenoidFireTime[BALL_SEARCH_RIGHT_SLING_INDEX] + 150)) break;
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
      PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_LEFT_SLING_TOP:
    case SW_RIGHT_SLING_TOP:
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
      PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_DROP_1_1:
    case SW_DROP_1_2:
    case SW_DROP_1_3:
      if (HandleDropTarget(0, switchHit)) {
        LoopMultiballCounter = 0;
        LoopMultiballJackpotReady = true;
        if (LastRightInlane) AwardCombo(COMBO_RIGHT_TO_UPPER);
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_DROP_2_1:
    case SW_DROP_2_2:
    case SW_DROP_2_3:
      if (HandleDropTarget(1, switchHit)) {
        if (LastLeftInlane) AwardCombo(COMBO_LEFT_TO_2BANK);
        else if (LastRightInlane) AwardCombo(COMBO_RIGHT_TO_2BANK);
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_DROP_3_1:
    case SW_DROP_3_2:
    case SW_DROP_3_3:
      if (HandleDropTarget(2, switchHit)) {
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_TARGET_1:
    case SW_TARGET_2:
    case SW_TARGET_3:
    case SW_TARGET_4:
    case SW_TARGET_5:
    case SW_TARGET_6:
    case SW_TARGET_7:
      LoopMultiballCounter = 0;
      LoopMultiballJackpotReady = true;
      HandleStandupTarget(switchHit);
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;
      
    case SW_TARGET_8:
    case SW_TARGET_9:      
      HandleStandupTarget(switchHit);
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_LOOP:
      if (LastTurnaroundHit==0) {
        LastTurnaroundHit = CurrentTime;
        HandleLoopMultiball();
        HandleLoopSpinnerAlternation(SW_LOOP);
        if (LoopMultiballRunning) LoopMultiballDoubleJackpotReady = true;
        if (SpinnerMultiballRunning) SpinnerMultiballDoubleJackpotReady = true;
        if (StormMultiballRunning && StormMultiballMegaJackpotReady) {
          if (Saucer1ScheduledEject && Saucer2ScheduledEject) {
            StormMultiballMegaJackpotReady = false;
            QueueNotification(SOUND_EFFECT_VP_MEGA_JACKPOT, 10);
            StartScoreAnimation(PlayfieldMultiplier * 10 * STORM_MULTIBALL_JACKPOT_VALUE);            
          }
        }
        if (BonusXIncreaseAvailable[CurrentPlayer]) {
          BonusXIncreaseAvailable[CurrentPlayer] -= 1;
          IncreaseBonusX();
        }
        if (LastLeftInlane) AwardCombo(COMBO_LEFT_TO_LOOP);
        else if (LastRightInlane) AwardCombo(COMBO_RIGHT_TO_LOOP);
        if (SpinnerFrenzyEndTime) {
          CurrentScores[CurrentPlayer] += 10000 * PlayfieldMultiplier;
          PlaySoundEffect(SOUND_EFFECT_LOOP_LIT);
        } else {
          CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
          PlaySoundEffect(SOUND_EFFECT_LOOP);
        }
      }
      
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_SPINNER:
      HandleSpinnerMultiball();
      HandleLoopSpinnerAlternation(SW_SPINNER);
      if (LastLeftInlane) AwardCombo(COMBO_LEFT_TO_SPINNER);
      if (SpinnerMultiballRunning) {
        if (Saucer2ScheduledEject==0) {
          CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
        } else {
          CurrentScores[CurrentPlayer] += 10000 * PlayfieldMultiplier;
        }
        PlaySoundEffect(SOUND_EFFECT_SPINNER_MAX);
      } else if (SpinnerFrenzyEndTime) {
        CurrentScores[CurrentPlayer] += 2500 * PlayfieldMultiplier;
        PlaySoundEffect(SOUND_EFFECT_SPINNER_LIT);
      } else if (SpinnerLitEndTime) {
        CurrentScores[CurrentPlayer] += 1000 * PlayfieldMultiplier;
        PlaySoundEffect(SOUND_EFFECT_SPINNER_LIT);
        if ( (CurrentTime+500)>SpinnerLitEndTime ) {
          SpinnerLitEndTime += 500;
        }
      } else {
        CurrentScores[CurrentPlayer] += 100 * PlayfieldMultiplier;
        PlaySoundEffect(SOUND_EFFECT_SPINNER);
      }      
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;
/*
    case SW_SAUCER_1:
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
//      PlaySoundEffect(SOUND_EFFECT_SAUCER);
      RPU_PushToTimedSolenoidStack(SOL_SAUCER_1, 16, CurrentTime+1000, true);
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;
*/
    case SW_LEFT_RETURN_LANE_TOP:
      LoopMultiballCounter = 0;
      if (ReturnLaneStatus[CurrentPlayer]&0x01) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 3000;
        PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
      } else {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 300;
        ReturnLaneStatus[CurrentPlayer] |= 0x01;
        PlaySoundEffect(SOUND_EFFECT_INLANE);
      }      
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_RIGHT_RETURN_LANE_TOP:
      LoopMultiballCounter = 0;
      if (ReturnLaneStatus[CurrentPlayer]&0x02) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 3000;
        PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
      } else {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 300;
        ReturnLaneStatus[CurrentPlayer] |= 0x02;
        PlaySoundEffect(SOUND_EFFECT_INLANE);
      }      
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_LEFT_INLANE_BOTTOM:
      if (LastRightInlane) {
        AwardCombo(COMBO_RIGHT_TO_LEFT);
        LastRightInlane = 0;
      }
      LastLeftInlane = CurrentTime;
      if (ReturnLaneStatus[CurrentPlayer]&0x04) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 3000;
        PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
      } else {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 300;
        ReturnLaneStatus[CurrentPlayer] |= 0x04;
        PlaySoundEffect(SOUND_EFFECT_INLANE);
      }

      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_RIGHT_INLANE_BOTTOM:
      if (LastLeftInlane) {
        AwardCombo(COMBO_LEFT_TO_RIGHT);
        LastLeftInlane = 0;
      }
      LastRightInlane = CurrentTime;
      if (ReturnLaneStatus[CurrentPlayer]&0x08) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 3000;
        PlaySoundEffect(SOUND_EFFECT_INLANE_LIT);
      } else {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 300;
        ReturnLaneStatus[CurrentPlayer] |= 0x08;
        PlaySoundEffect(SOUND_EFFECT_INLANE);
      }

      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;      

    case SW_OUTLANES_BOTTOM:
      if (BallSaveEndTime) BallSaveEndTime += 3000;
//      PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;
  }


}


int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(0xFF, false, false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    STORM23_SetDisplayCredits(Credits, !FreePlayMode);

    if (SamePlayerShootsAgain) {
      QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
//        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          RPU_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  UpdateLockStatus();
  MoveBallFromOutholeToRamp();

  byte switchHit;
  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;

  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    returnState = HandleSystemSwitches(curState, switchHit);
    if (NumTiltWarnings <= MaxTiltWarnings) HandleGamePlaySwitches(switchHit);
  }

  if (CreditResetPressStarted) {
    if (CurrentBallInPlay < 2) {
      // If we haven't finished the first ball, we can add players
      AddPlayer();
      if (DEBUG_MESSAGES) {
        Serial.write("Start game button pressed\n\r");
      }
      CreditResetPressStarted = 0;
    } else {
      if (RPU_ReadSingleSwitchState(SW_CREDIT_RESET)) {
        if (TimeRequiredToResetGame != 99 && (CurrentTime - CreditResetPressStarted) >= ((unsigned long)TimeRequiredToResetGame*1000)) {
          // If the first ball is over, pressing start again resets the game
          if (Credits >= 1 || FreePlayMode) {
            if (!FreePlayMode) {
              Credits -= 1;
              RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
              STORM23_SetDisplayCredits(Credits, !FreePlayMode);
            }
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
            CreditResetPressStarted = 0;
          }
        }
      } else {
        CreditResetPressStarted = 0;
      }
    }

  }

  if (lastBallFirstSwitchHitTime==0 && BallFirstSwitchHitTime!=0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)BallSaveNumSeconds)*1000;
  }
  if (CurrentTime>(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > RPU_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= RPU_OS_MAX_DISPLAY_SCORE;
    if (!TournamentScoring) AddSpecialCredit();
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (!ExtraBallCollected) {
            AwardExtraBall();
          }
        }
      }
    }

  }

  return returnState;
}


unsigned long LastLEDUpdateTime = 0;
byte LEDPhase = 0;
unsigned long NumLoops = 0;
unsigned long LastLoopReportTime = 0;

void loop() {

  if (0 && DEBUG_MESSAGES) {
    NumLoops += 1;
    if (CurrentTime>(LastLoopReportTime+1000)) {
      LastLoopReportTime = CurrentTime;
      char buf[128];
      sprintf(buf, "Loop running at %lu Hz\n", NumLoops);
      Serial.write(buf);
      NumLoops = 0;
    }
  }

  
  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_DIAGNOSTICS) {
    newMachineState = RunDiagnosticsMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_Update(CurrentTime);
  Audio.Update(CurrentTime);

/*
  if (LastLEDUpdateTime == 0 || (CurrentTime - LastLEDUpdateTime) > 250) {
    LastLEDUpdateTime = CurrentTime;
    RPU_SetBoardLEDs((LEDPhase % 8) == 1 || (LEDPhase % 8) == 3, (LEDPhase % 8) == 5 || (LEDPhase % 8) == 7);
    LEDPhase += 1;
  }
*/  
}
