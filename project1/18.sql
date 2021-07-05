SELECT AVG(cp.level) AS 'AVERAGE LEVEL'
FROM Gym g, CatchedPokemon cp
WHERE g.leader_id = cp.owner_id