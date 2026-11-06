/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   acquire.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yourlogin <yourlogin@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/30 00:00:00 by yourlogin         #+#    #+#             */
/*   Updated: 2026/10/30 00:00:00 by yourlogin        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	queue_request_locked(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	coder->request_seq = ++sim->seq;
	coder->request_deadline = coder->last_start + sim->cfg.burnout;
	lock_pair(sim, coder->left, coder->right);
	heap_push(sim, &sim->dongles[coder->left].queue, coder->id - 1);
	heap_push(sim, &sim->dongles[coder->right].queue, coder->id - 1);
	unlock_pair(sim, coder->left, coder->right);
	pthread_cond_broadcast(&sim->changed);
}

static void	single_coder_locked(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	pthread_mutex_lock(&sim->dongles[0].mutex);
	sim->dongles[0].owner = coder->id;
	pthread_mutex_unlock(&sim->dongles[0].mutex);
	log_locked(sim, coder->id, "has taken a dongle");
	while (!sim->stop)
		pthread_cond_wait(&sim->changed, &sim->mutex);
}

static void	await_pair_locked(t_coder *coder)
{
	t_sim		*sim;
	long long	wait_ms;

	sim = coder->sim;
	queue_request_locked(coder);
	while (!sim->stop)
	{
		if (try_grant_locked(coder))
			break ;
		wait_ms = next_cooldown_locked(coder);
		if (wait_ms > 0)
			wait_delta_locked(sim, wait_ms);
		else
			pthread_cond_wait(&sim->changed, &sim->mutex);
	}
}

int	request_dongles(t_coder *coder)
{
	t_sim	*sim;
	int		active;

	sim = coder->sim;
	pthread_mutex_lock(&sim->mutex);
	if (!sim->stop && coder->compiles < sim->cfg.required
		&& sim->cfg.count == 1)
		single_coder_locked(coder);
	else if (!sim->stop && coder->compiles < sim->cfg.required)
		await_pair_locked(coder);
	active = !sim->stop && coder->compiles < sim->cfg.required;
	pthread_mutex_unlock(&sim->mutex);
	return (active);
}
