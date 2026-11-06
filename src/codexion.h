/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yourlogin <yourlogin@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/30 00:00:00 by yourlogin         #+#    #+#             */
/*   Updated: 2026/10/30 00:00:00 by yourlogin        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <time.h>
# include <stddef.h>

typedef enum e_policy
{
	FIFO,
	EDF
}	t_policy;

typedef struct s_config
{
	int		count;
	int		burnout;
	int		compile;
	int		debug;
	int		refactor;
	int		required;
	int		cooldown;
	t_policy	policy;
}	t_config;

typedef struct s_heap
{
	int		*ids;
	int		size;
	int		capacity;
}	t_heap;

typedef struct s_dongle
{
	pthread_mutex_t	mutex;
	long long		ready_at;
	int				owner;
	t_heap			queue;
}	t_dongle;

struct s_sim;

typedef struct s_coder
{
	int				id;
	int				left;
	int				right;
	int				compiles;
	long long		last_start;
	long long		request_seq;
	long long		request_deadline;
	pthread_t		thread;
	struct s_sim	*sim;
}	t_coder;

typedef struct s_sim
{
	t_config		cfg;
	t_dongle		*dongles;
	t_coder			*coders;
	pthread_mutex_t	mutex;
	pthread_mutex_t	print_mutex;
	pthread_cond_t	changed;
	pthread_t		monitor;
	long long		start;
	long long		cycle;
	long long		seq;
	int				ready;
	int				stop;
	int				mutex_ready;
	int				print_ready;
	int				cond_ready;
	int				dongles_ready;
	int				coders_created;
	int				monitor_created;
}	t_sim;

/* Parsing, allocation, lifecycle. */
int			parse_args(int argc, char **argv, t_config *cfg);
int			init_sim(t_sim *sim, t_config *cfg);
void		cleanup_sim(t_sim *sim);
int			run_sim(t_sim *sim);

/* Time and synchronized output; caller holds sim->mutex for logging. */
long long	now_ms(void);
void		wait_delta_locked(t_sim *sim, long long delta);
int			wait_phase(t_sim *sim, int duration);
void		log_locked(t_sim *sim, int id, char *message);

/* A real binary heap on each dongle's arbitration queue. */
int			heap_before(t_sim *sim, int a, int b);
void		heap_swap(int *a, int *b);
void		heap_push(t_sim *sim, t_heap *heap, int id);
int			heap_top(t_heap *heap);
int			heap_pop(t_sim *sim, t_heap *heap);
int			heap_remove(t_heap *heap, int id);

/* Resource arbitration: mutex ordering is always ascending index. */
void		lock_pair(t_sim *sim, int left, int right);
void		unlock_pair(t_sim *sim, int left, int right);
int			request_dongles(t_coder *coder);
long long	edf_slot(t_coder *coder);
void		init_schedule(t_sim *sim);
int			try_grant_locked(t_coder *coder);
long long	next_cooldown_locked(t_coder *coder);
void		release_dongles_locked(t_coder *coder);

/* Worker/monitor thread entry points. */
int			begin_refactor(t_coder *coder);
void		*coder_thread(void *arg);
void		*monitor_thread(void *arg);

#endif
