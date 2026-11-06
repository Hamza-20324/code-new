/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yourlogin <yourlogin@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/30 00:00:00 by yourlogin         #+#    #+#             */
/*   Updated: 2026/10/30 00:00:00 by yourlogin        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <unistd.h>
#include <string.h>

static int	error_message(char *message)
{
	write(2, message, strlen(message));
	return (1);
}

int	main(int argc, char **argv)
{
	t_config	cfg;
	t_sim		sim;
	int			ok;

	if (!parse_args(argc, argv, &cfg))
		return (error_message("Error: invalid arguments\n"));
	if (!init_sim(&sim, &cfg))
	{
		cleanup_sim(&sim);
		return (error_message("Error: initialization failed\n"));
	}
	ok = run_sim(&sim);
	cleanup_sim(&sim);
	if (!ok)
		return (error_message("Error: thread creation failed\n"));
	return (0);
}
