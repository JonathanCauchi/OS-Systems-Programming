int main(int argc, char **argv) {
	char *message = "This is a system call test!\n";
	int i; for (i=0; message[i]!='\n';i++);

	__asm__ volatile ("int $0x80"
		: 
		: "a" (4), "b" (1), "c" (message), "d" (i));

	__asm__ volatile ("int $0x80"
		: 
		: "a" (1), "b" (0));
}