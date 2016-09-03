/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2016 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file g_skillrating.c
 * @brief Bayesian skill rating
 */
#ifdef FEATURE_RATING

#include "g_local.h"

void G_UpdateSkillRating(int winner);

// MU      25            - mean
// SIGMA   MU / 3        - standard deviation
// BETA    SIGMA / 2     - skill chain length
// TAU     SIGMA / 100   - dynamics factor
// EPSILON 0.f           - draw margin (assumed null)

/**
 * @brief Calculate skill ratings
 * @details Rate players in this map based on team performance
 */
void G_CalculateSkillRatings(void)
{
	char cs[MAX_STRING_CHARS];
	char *buf;
	int  winner;

	// determine winner
	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
	buf    = Info_ValueForKey(cs, "w");
	winner = atoi(buf);

	// change from scripting value for winner to spawnflag value
	switch (winner)
	{
	case 0: // AXIS
		winner = TEAM_AXIS;
		break;
	case 1: // ALLIES
		winner = TEAM_ALLIES;
		break;
	default:
		break;
	}

	// log
	G_Printf("SKILL_RATING: Map: %s, Winner: %d, Time: %d, Timelimit: %d\n",
	         level.rawmapname, winner, level.timeCurrent - level.startTime, g_timelimit.integer * 60000);
	G_UpdateSkillRating(winner);
}

/**
 * @brief Probability density function of standard normal distribution
 * @details Relative likelihood for a random variable x to take on a given value
 */
float pdf(float x)
{
	return exp(-0.5 * pow(x, 2)) / sqrt(2 * M_PI);
}

/**
 * @brief Cumulative distribution function of standard normal distribution
 * @details Probability that a real-valued random variable with a given probability
 *          distribution will be found to have a value less than or equal to x
 */
float cdf(float x)
{
	return 0.5 * (1 + erff(x / sqrt(2)));
}

/**
 * @brief Mean additive truncated Gaussian function (non-draw version)
 * @details How much to update the mean after a win or loss
 */
float V(float t, float epsilon)
{
	return pdf(t - epsilon) / cdf(t - epsilon);
}

/**
 * @brief Variance multiplicative function (non-draw version)
 * @details  How much to update the standard deviation after a win or loss
 */
float W(float t, float epsilon)
{
	return V(t, epsilon) * (V(t, epsilon) + t - epsilon);
}

/**
 * @brief Map winning probability
 * @details Get wining parameter bias of the played map
 */
float G_MapWinProb(int team)
{
	// FIXME
	return 0.5f;
}

/**
 * @brief Update skill rating
 * @details Update player's skill rating based on team performance
 */
void G_UpdateSkillRating(int winner)
{
	gclient_t *cl;

	float teamMuX      = 0;
	float teamMuL      = 0;
	float teamSigmaSqX = 0;
	float teamSigmaSqL = 0;
	int   numPlayersX  = 0;
	int   numPlayersL  = 0;
	float c, v, w, t, winningMu, losingMu, muFactor, sigmaFactor;
	float mapProb, mapMu, mapSigma, mapBeta;
	int   playerTeam, rankFactor, i;

	// total play time
	int totalTime = level.intermissiontime - level.startTime - level.timeDelta;

	// map side parameter
	if (g_skillRating.integer > 1)
	{
		mapProb  = G_MapWinProb(winner);
		mapMu    = 2 * MU * mapProb;
		mapSigma = 2 * MU * sqrt(mapProb * (1.0f - mapProb));
		mapBeta  = mapSigma / 2;
	}

	// player additive factors
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		// player has not played at all
		if (cl->sess.time_axis == 0 && cl->sess.time_allies == 0)
		{
			continue;
		}

		// player has played in at least one of the team
		if (cl->sess.time_axis > 0)
		{
			teamMuX      += cl->sess.mu * (cl->sess.time_axis / (float)totalTime);
			teamSigmaSqX += pow(cl->sess.sigma, 2);
			numPlayersX++;
		}
		if (cl->sess.time_allies > 0)
		{
			teamMuL      += cl->sess.mu * (cl->sess.time_allies / (float)totalTime);
			teamSigmaSqL += pow(cl->sess.sigma, 2);
			numPlayersL++;
		}
	}

	// normalizing constant
	if (g_skillRating.integer > 1)
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2) + pow(mapSigma, 2) + pow(mapBeta, 2));
	}
	else
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2));
	}

	// determine teams rank
	winningMu = (winner == TEAM_AXIS) ? teamMuX : teamMuL;
	losingMu  = (winner == TEAM_AXIS) ? teamMuL : teamMuX;

	// map bias
	if (g_skillRating.integer > 1)
	{
		if (mapProb > 0.5f)
		{
			winningMu += mapMu;
		}
		else if (mapProb < 0.5f)
		{
			losingMu += 2 * MU - mapMu;
		}
	}

	// team performance
	t = (winningMu - losingMu) / c;

	// truncated Gaussian correction
	v = V(t, EPSILON / c);
	w = W(t, EPSILON / c);

	// update players rating
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		// player has not played at all
		if (cl->sess.time_axis == 0 && cl->sess.time_allies == 0)
		{
			continue;
		}

		// find which is team even when player has played on both side or has moved to spectator
		if (cl->sess.time_axis - cl->sess.time_allies > 0)
		{
			playerTeam = TEAM_AXIS;
		}
		else if (cl->sess.time_allies - cl->sess.time_axis > 0)
		{
			playerTeam = TEAM_ALLIES;
		}
		else
		{
			// player has played exact same time in each team
			continue;
		}

		// factors
		muFactor    = (pow(cl->sess.sigma, 2) + pow(TAU, 2)) / c;
		sigmaFactor = (pow(cl->sess.sigma, 2) + pow(TAU, 2)) / pow(c, 2);
		rankFactor  = (playerTeam == winner) ? 1 : -1;

		// rating update
		cl->sess.mu    = cl->sess.mu + rankFactor * muFactor * v;
		cl->sess.sigma = sqrt((pow(cl->sess.sigma, 2) + pow(TAU, 2)) * (1 - sigmaFactor * w));
	}
}

/**
 * @brief Calculate win probability
 * @details Calculate win probability // Axis = winprob, Allies = 1.0 - winprob
 */
float G_CalculateWinProbability(int team)
{
	gclient_t *cl;

	float teamMuX      = 0;
	float teamMuL      = 0;
	float teamSigmaSqX = 0;
	float teamSigmaSqL = 0;
	int   numPlayersX  = 0;
	int   numPlayersL  = 0;
	float c, t, winningMu, losingMu;
	float mapProb, mapMu, mapSigma, mapBeta;
	int   i;

	// current play time
	int currentTime = level.timeCurrent - level.startTime - level.timeDelta;

	// map side parameter
	if (g_skillRating.integer > 1)
	{
		mapProb  = G_MapWinProb(team);
		mapMu    = 2 * MU * mapProb;
		mapSigma = 2 * MU * sqrt(mapProb * (1.0f - mapProb));
		mapBeta  = mapSigma / 2;
	}

	// player additive factors
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		if (g_gamestate.integer == GS_PLAYING)
		{
			// player has not played at all
			if (cl->sess.time_axis == 0 && cl->sess.time_allies == 0)
			{
				continue;
			}

			// player has played in at least one of the team
			if (cl->sess.time_axis > 0)
			{
				teamMuX      += cl->sess.mu * (cl->sess.time_axis / (float)currentTime);
				teamSigmaSqX += pow(cl->sess.sigma, 2);
				numPlayersX++;
			}
			if (cl->sess.time_allies > 0)
			{
				teamMuL      += cl->sess.mu * (cl->sess.time_allies / (float)currentTime);
				teamSigmaSqL += pow(cl->sess.sigma, 2);
				numPlayersL++;
			}
		}
		// warmup and intermission
		else
		{
			// avoid nan while players join teams
			if (level.numPlayingClients < 2)
			{
				return 0.5f;
			}

			// check actual team only
			if (cl->sess.sessionTeam == TEAM_AXIS)
			{
				teamMuX      += cl->sess.mu;
				teamSigmaSqX += pow(cl->sess.sigma, 2);
				numPlayersX++;
			}
			if (cl->sess.sessionTeam == TEAM_ALLIES)
			{
				teamMuL      += cl->sess.mu;
				teamSigmaSqL += pow(cl->sess.sigma, 2);
				numPlayersL++;
			}
		}
	}

	// normalizing constant
	if (g_skillRating.integer > 1)
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2) + pow(mapSigma, 2) + pow(mapBeta, 2));
	}
	else
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2));
	}

	// determine teams rank
	winningMu = (team == TEAM_AXIS) ? teamMuX : teamMuL;
	losingMu  = (team == TEAM_AXIS) ? teamMuL : teamMuX;

	// map bias
	if (g_skillRating.integer > 1)
	{
		if (mapProb > 0.5f)
		{
			winningMu += mapMu;
		}
		else if (mapProb < 0.5f)
		{
			losingMu += 2 * MU - mapMu;
		}
	}

	// team performance
	t = (winningMu - losingMu - EPSILON) / c;

	return cdf(t);
}

#endif
