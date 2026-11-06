/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   grant.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhamza <zhamza@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/06 02:37:26 by zhamza            #+#    #+#             */
/*   Updated: 2026/11/06 02:37:54 by zhamza           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	pair_available_locked(t_coder *coder, long long now)
{
	t_sim		*sim;
	t_dongle	*a;
	t_dongle	*b;
	int			available;

	sim = coder->sim;
	lock_pair(sim, coder->left, coder->right);
	a = &sim->dongles[coder->left];
	b = &sim->dongles[coder->right];
	available = (a->owner == -1 && b->owner == -1
			&& now >= a->ready_at && now >= b->ready_at);
	unlock_pair(sim, coder->left, coder->right);
	return (available);
}

static int	priority_ok_locked(t_coder *coder, int dongle, long long now)
{
	t_sim	*sim;
	t_heap	*queue;
	int		i;
	int		other;

	sim = coder->sim;
	queue = &sim->dongles[dongle].queue;
	i = 0;
	while (i < queue->size)
	{
		other = queue->ids[i++];
		if (other != coder->id - 1
			&& heap_before(sim, other, coder->id - 1)
			&& pair_available_locked(&sim->coders[other], now))
			return (0);
	}
	return (1);
}

static int	can_grant_locked(t_coder *coder, long long now)
{
	if (!pair_available_locked(coder, now))
		return (0);
	if (!priority_ok_locked(coder, coder->left, now))
		return (0);
	return (priority_ok_locked(coder, coder->right, now));
}

static void	commit_grant_locked(t_coder *coder, long long start)
{
	t_sim	*sim;

	sim = coder->sim;
	heap_remove(&sim->dongles[coder->left].queue, coder->id - 1);
	heap_remove(&sim->dongles[coder->right].queue, coder->id - 1);
	sim->dongles[coder->left].owner = coder->id;
	sim->dongles[coder->right].owner = coder->id;
	coder->last_start = start;
}

int	try_grant_locked(t_coder *coder)
{
	t_sim		*sim;
	long long	start;

	sim = coder->sim;
	start = now_ms();
	if (start >= coder->last_start + sim->cfg.burnout
		|| !can_grant_locked(coder, start))
		return (0);
	lock_pair(sim, coder->left, coder->right);
	start = now_ms();
	if (start >= coder->last_start + sim->cfg.burnout)
	{
		unlock_pair(sim, coder->left, coder->right);
		return (0);
	}
	commit_grant_locked(coder, start);
	unlock_pair(sim, coder->left, coder->right);
	log_locked(sim, coder->id, "has taken a dongle");
	log_locked(sim, coder->id, "has taken a dongle");
	log_locked(sim, coder->id, "is compiling");
	pthread_cond_broadcast(&sim->changed);
	return (1);
}
