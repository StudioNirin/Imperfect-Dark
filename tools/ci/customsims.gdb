# Seeded smoke match for the three custom simulants, run by customsims.sh.
# Types are patched into the first three bot configs at frame 30 so the match
# itself is the stock seeded 0x32 one. Every 600 frames log the types and the
# targets so a crash or a target-selection failure shows up as a missing line.
set pagination off
set confirm off
handle SIGPIPE SIGUSR1 SIGUSR2 nostop noprint

break lvTick if g_Vars.lvframe60 == 30
commands
	silent
	set var g_BotConfigsArray[0].type = 13
	set var g_BotConfigsArray[1].type = 14
	set var g_BotConfigsArray[2].type = 15
	printf "CUSTOMSIM types %d %d %d\n", g_BotConfigsArray[0].type, g_BotConfigsArray[1].type, g_BotConfigsArray[2].type
	continue
end

break lvTick if g_Vars.lvframe60 % 600 == 0
commands
	silent
	printf "CUSTOMSIM f%d types=%d,%d,%d targets=%d,%d,%d gun=%d patience=%d surgedmg=%.2f\n", g_Vars.lvframe60, g_BotConfigsArray[0].type, g_BotConfigsArray[1].type, g_BotConfigsArray[2].type, g_MpBotChrPtrs[0]->target, g_MpBotChrPtrs[1]->target, g_MpBotChrPtrs[2]->target, botGetWeaponNum(g_MpBotChrPtrs[0]), g_MpBotChrPtrs[1]->aibot->vulturepatience60, botGetStatScale(g_MpBotChrPtrs[2], 3)
	continue
end

run
