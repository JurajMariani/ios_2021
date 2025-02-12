
#include "santa_and_helpers.h"

void init_santa(process_t* santa)
{

    santa->name = "Santa";
    santa->init_msg = "going to sleep";
	santa->help_msg = "helping elves";
	santa->done_msg = "closing workshop";
	santa->end_msg = "Christmas started";
    santa->ID = 1;

}

void init_elf(process_t* elf, int id)
{

    elf->name = "Elf";
    elf->init_msg = "started";
	elf->help_msg = "need help";
	elf->done_msg = "get help";
	elf->end_msg = "taking holidays";
    elf->ID = id;

}

void init_reindeer(process_t* reindeer, int id)
{

    reindeer->name = "RD";
    reindeer->init_msg = "rstarted";
	reindeer->help_msg = "";
	reindeer->done_msg = "return home";
	reindeer->end_msg = "get hitched";
    reindeer->ID = id;

}