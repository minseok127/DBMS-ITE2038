SELECT name
FROM Pokemon
WHERE type in (
  SELECT c1.type
  FROM (
    SELECT type, COUNT(*) AS 'Count'
    FROM Pokemon
    GROUP BY type
    ORDER BY COUNT(*) DESC ) c1
  WHERE c1.Count in (
    SELECT tmp.Count
    FROM (
      SELECT c2.Count
      FROM (
        SELECT type, COUNT(*) AS 'Count'
        FROM Pokemon
        GROUP BY type
        ORDER BY COUNT(*) DESC ) c2
      GROUP BY Count
      ORDER BY Count DESC
      LIMIT 2 ) AS tmp ))
ORDER BY name