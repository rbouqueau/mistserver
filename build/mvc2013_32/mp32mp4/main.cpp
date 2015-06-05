int input(int argc, char * argv[]);
int output(int argc, char * argv[]);

int main(int argc, char * argv[]) {
	int argc_i = 3;
	char *argv_i[] = { "-s", "r", "santanasazi.mp3" };
	//int argc_i = 1;
	//char *argv_i[] = { "--help"};
	input(argc_i, argv_i);

	int argc_o = 2;
	char *argv_o[] = { "-s", "r" };
	return output(argc_o, argv_o);
}
