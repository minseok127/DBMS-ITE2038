SELECT t1.name AS 'Leader Name', AVG(cp.level) AS 'Average Pokemon Level'
FROM CatchedPokemon cp, Trainer t1
WHERE cp.owner_id = t1.id AND
      EXISTS (
        SELECT g.leader_id
        FROM Gym g
        WHERE t1.id = g.leader_id )
GROUP BY owner_id
ORDER BY t1.name