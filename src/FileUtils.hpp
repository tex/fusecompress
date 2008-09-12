struct stat;

class FileUtils
{
	/*
	 * Change the access rights of the file specified
	 * by name, open it and then return original
	 * access rights back.
	 */
	static int force(const char *name, const struct stat &buf);
public:
	/*
	 * Open a regular file specified by name. If the caller has
	 * insufficient rights, the function tries to
	 * change the access rights of the file, open it and
	 * then return the original rights back.
	 */
	static int open(const char *name);
};

