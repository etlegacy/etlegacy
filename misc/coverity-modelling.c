// add function calls that need to be modelled out
// Coverity calls can be found here: https://scan.coverity.com/models#c_checker_checkerconfig

void make_error_disapear(const char *msg)
{
	__coverity_panic__();
}
