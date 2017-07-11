
#include <stdio.h>

#include "utils.h"
#include "mlsdr.h"

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	struct mlsdr_desc *desc = mlsdr_enumerate();
	while (desc != NULL) {
		printf("%s %s %s\n", desc->description, desc->manufacturer, desc->serno);
		desc = desc->next;
	}

	return EXIT_SUCCESS;
}
