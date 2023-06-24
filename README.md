# Storm23  
All new code that can run on Stern Lightning (1981)
  
## How to build this for your machine  
1) build or buy a board to interface an Arduino MEGA 2560 PRO with the MPU processor socket or J5 connector  
2) get this code, put it in a Storm23 folder  
3) update configuration in RPU_config.h to match your board type  
4) compile/install code on the Arduino MEGA 2560 PRO
5) Example audio files*:  https://drive.google.com/file/d/1mYQ0klncc1H2uyhFZB6mupIjTGjiKkT6/view?usp=sharing

     * Music by Karl Casey @ White Bat Audio
  
## More information
http://pinballrefresh.com  
  
## Test / Audit / Settings from coin-door switch
Tests (test number shown in Credits, Ball in Play is blank)  
1 - Lamps  
2 - Displays  
3 - Solenoids  
4 - Switches  
5 - Sounds (not applicable)  

Settings & Audits (page number shown in Ball in Play, Credits is blank)  
1 - Award Score 1  
2 - Award Score 2  
3 - Award Score 3  
4 - High Score  
5 - Credits  
6 - Total Plays  
7 - Total Replays  
8 - High Score Beat  
9 - Chute 2 Coins  
10 - Chute 1 Coins  
11 - Chute 3 Coins  
12 - Reboot (All displays show 8007 (as in "BOOT"), and Credit/Reset button restarts)  
13 - Coins per Credit for Chute 1  
14 - Coins per Credit for Chute 2  
15 - Coins per Credit for Chute 3  
16 - Free Play  
17 - Ball Save  
18 - Sound Selector  
19 - Music Volume  
20 - Sound Effects Volume  
21 - Callouts Volume  
22 - Tournament Scoring  
23 - Tilt Warnings  
24 - Award Scores (0 = all extra balls, 7 = all specals)  
25 - Number of Balls Per Game  
26 - Scrolling Scores  
27 - Extra Ball Award (for tournament scoring)  
28 - Special Award (for tournament scoring)  
29 - Credit/Reset hold time for restart (0=immediate, 1, 2, 3 seconds, or 99 for never)  
30 - Sound Track 1 or 2  
31 - Saucer Eject (1=light, 2=normal, 3=strong)  
32 - Multiball Ball Save* (0, 5, 10, 15, 20 seconds)  
33 - Spins to Start Spinner Multiball  
  
* Warning: because this machine lacks auto-plunge, multiball ball save will serve a ball into the   
  shooter lane during the ball save period and the player will have no incentive to put that ball into   
  play until they've drained the remaining balls.

# Storm 23 Rules  

## Multiballs
Complete 5 standups (3-7) to qualify lock #1  
Complete 2 & 2 (1, 2, 8, and 9) standups to qualify lock #2  
Timers will not expire during any multiball  
  
### Multiball #1 (2 ball) -   
	with lock #1 (left saucer), complete loop 3 times without engaging the upper playfield  
	on 3rd completion, lock #1 is released. (number of completions increases with each multiball)  
	playfield multiplier is increased by 2 for 30 seconds  
	loop for jackpot / re/qualified with upper & lower alternation  
	loop for triple jackpot during saucer hold  
	top saucer for double jackpot - ejects after 10 seconds (requalify with loop)  
	+ bonus x available on loop for each drop bank completed  
  
### Multiball #2 (2 ball) -   
	with lock #2 (right saucer), get 50 spins to eject lock #2  
	playfield multiplier is increased by 2 for 30 seconds  
	spinner = 5k * playfield multiplier  
	right saucer for double jackpot - ejects after 10 seconds (re/qualify with loop)  
 	spinner = 10k * playfield multiplier during saucer hold  
	+ bonus x available on loop for each drop bank completed  

### Multiball #3 (3 ball) -  
	With 2 balls locked, hit strobing lamp to start multiball (moves between 9 standups) 
	all drop targets reset  
	playfield multiplier is increased by 3 for 45 seconds  
	landing a saucer = jackpot (holds for 15 seconds)  
	landing second saucer = super jackpot (holds for 15 seconds)  
	if two balls are in play and both are in saucers -- add a ball  
	all drops down during multiball = double jackpot  
	two balls in saucers and loop = mega jackpot  
	  
### Jailbreak Multiball -  
	Knocking a locked ball from saucer  
	+ Playfield mulitplier for 60 seconds  
	all standups lit  
  

## Modes  
Timers add together if multiple modes are started at once  
  
### Drop Frenzy  
	Finish bank 1, then 2, then 3 for drop frenzy  
	30 seconds  
	+1x playfield  
	All drops worth 10k and reset 3 seconds after being hit  
  
### Drop Jackpot (after completing Drop Frenzy)  
	After drop frenzy, drops reset   
	Drop bank 1 has to be hit first. If any of the 1s drops are up, 2s and 3s give no points and reset after 5 seconds  
	Once 1s are done, 2s can be done  
	After 3s are completed, player gets a jackpot of 100k  
  
### Drop Double Jackpot (after completing Drop Jackpot)  
	Same rules as drop jackpot except targets reset after 30 seconds and player must start over with 1s  
  
### Drop Super & Mega (as above, with less time)  
  

### Turnaround / Spinner Alternation (aka Spinner frenzy)  
	Starts with turnaround (lights spinner)  
	Spinner lights turnaround  
	Alternate 3 times (6 hits) for spinner/turnaround frenzy  
	2.5k per spin  
	20k per turnaround  
	30 seconds  
	+1x playfield  
  
### Combos  
	Left inlane - turnaround  
	Left inlane - spinner  
	Left inlane - 2s drop  
	Left to right  
	right inlane - turnaround  
	right inlane - 1s drop  
	right inlane - 2s drop  
	right to left  

  3 combos achieved = holdover bonus x  
  6 combos achieved = holdover bonus  
  8 combos achieved = double timers  
  
## Spinner Value  
	Unlit = 100  
	Lit = 1000  
	Frenzy = 2500  
	Spinner Multiball = 5000  
	Spinner Multiball during saucer hold = 10000  
  
  
  
