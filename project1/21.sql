SELECT t.name, COUNT(*) AS 'COUNT'
FROM Gym g, CatchedPokemon cp, Trainer t
WHERE g.leader_id = cp.owner_id AND t.id = g.leader_id
GROUP BY t.name
ORDER BY t.name