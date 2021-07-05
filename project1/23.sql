SELECT DISTINCT t.name
FROM Trainer t, CatchedPokemon cp
WHERE t.id = cp.owner_id AND cp.level <= 10
ORDER BY t.name