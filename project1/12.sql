SELECT DISTINCT p.name, p.type
FROM CatchedPokemon cp, Pokemon p
WHERE cp.pid = p.id AND cp.level >= 30
ORDER BY p.name