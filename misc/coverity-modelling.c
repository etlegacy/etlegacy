// add function calls that need to be modelled out
// Coverity calls can be found here: https://scan.coverity.com/models#c_checker_checkerconfig

void make_error_disappear(const char *msg)
{
	__coverity_panic__();
}

int rand(void)
{
	/* ignore */
	return 0;
}

long random(void)
{
	/* ignore */
	return 0L;
}

void srand(unsigned int seed)
{
	/* ignore */
	__coverity_tainted_data_sanitize__(seed);
}
