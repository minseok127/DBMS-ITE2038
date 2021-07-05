SELECT COUNT(*) AS 'COUNT'
FROM CatchedPokemon cp, Pokemon p
WHERE cp.pid = p.id
GROUP BY p.type
ORDER BY p.type