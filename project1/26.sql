SELECT p.name
FROM CatchedPokemon cp, Pokemon p
WHERE cp.pid = p.id AND cp.nickname LIKE '% %'
ORDER BY p.name DESC