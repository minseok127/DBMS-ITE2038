SELECT SUM(cp.level) AS 'SUM OF LEVEL'
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id AND t.name = 'Matis'