SELECT AVG(cp.level) AS 'AVG Level of Pokemon(RED)'
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id AND t.name = 'Red'