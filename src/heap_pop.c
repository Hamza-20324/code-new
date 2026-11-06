/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_pop.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yourlogin <yourlogin@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/30 00:00:00 by yourlogin         #+#    #+#             */
/*   Updated: 2026/10/30 00:00:00 by yourlogin        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	sift_down(t_sim *sim, t_heap *heap, int parent)
{
	int	child;
	int	best;

	while (parent * 2 + 1 < heap->size)
	{
		child = parent * 2 + 1;
		best = child;
		if (child + 1 < heap->size
			&& heap_before(sim, heap->ids[child + 1], heap->ids[child]))
			best = child + 1;
		if (!heap_before(sim, heap->ids[best], heap->ids[parent]))
			break ;
		heap_swap(&heap->ids[parent], &heap->ids[best]);
		parent = best;
	}
}

int	heap_pop(t_sim *sim, t_heap *heap)
{
	int	result;

	if (heap->size == 0)
		return (-1);
	result = heap->ids[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->ids[0] = heap->ids[heap->size];
		sift_down(sim, heap, 0);
	}
	return (result);
}

/* A dongle has at most two neighbors, so removing either entry keeps
 * its (at most one-element) remaining queue a valid binary heap. */
int	heap_remove(t_heap *heap, int id)
{
	int	i;

	i = 0;
	while (i < heap->size && heap->ids[i] != id)
		i++;
	if (i == heap->size)
		return (0);
	heap->size--;
	if (i < heap->size)
		heap->ids[i] = heap->ids[heap->size];
	return (1);
}
