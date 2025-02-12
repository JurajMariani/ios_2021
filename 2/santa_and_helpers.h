

typedef struct process{
	char* name;

	char* init_msg;
	char* help_msg;
	char* done_msg;
	char* end_msg;
	int ID;
}process_t;



void init_santa(process_t* santa);
void init_elf(process_t* elf, int id);
void init_reindeer(process_t* reindeer, int id);