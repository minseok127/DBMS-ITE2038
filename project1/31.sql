SELECT p.type
FROM Pokemon p, Evolution e
WHERE p.id = e.before_id
GROUP BY p.type
HAVING COUNT(*) >= 3
ORDER BY p.type DESC