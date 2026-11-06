/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhamza <zhamza@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/06 02:38:21 by zhamza            #+#    #+#             */
/*   Updated: 2026/11/06 02:38:24 by zhamza           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	earliest_burnout(t_sim *sim, long long *deadline)
{
	long long	candidate;
	int			id;
	int			i;

	id = -1;
	i = 0;
	*deadline = 9223372036854775807LL;
	while (i < sim->cfg.count)
	{
		candidate = sim->coders[i].last_start + sim->cfg.burnout;
		if (sim->coders[i].compiles < sim->cfg.required
			&& candidate < *deadline)
		{
			*deadline = candidate;
			id = i;
		}
		i++;
	}
	return (id);
}

static void	monitor_loop_locked(t_sim *sim)
{
	long long	deadline;
	long long	wait_ms;
	int			id;

	while (!sim->stop)
	{
		id = earliest_burnout(sim, &deadline);
		wait_ms = deadline - now_ms();
		if (wait_ms <= 0)
		{
			sim->stop = 1;
			log_locked(sim, id + 1, "burned out");
			pthread_cond_broadcast(&sim->changed);
			break ;
		}
		wait_delta_locked(sim, wait_ms);
	}
}

void	*monitor_thread(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	pthread_mutex_lock(&sim->mutex);
	while (!sim->ready)
		pthread_cond_wait(&sim->changed, &sim->mutex);
	monitor_loop_locked(sim);
	pthread_mutex_unlock(&sim->mutex);
	return (NULL);
}
