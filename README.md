

## Выбор конфигурации размеров буфера

Для сравнения эффективности IPC-механизмов (SHM, очереди сообщений System V, FIFO) при передаче файла объемом **4 ГБ** были выбраны три группы размеров буфера:

*   **«Малый размер» (8 КБ и 64 КБ)** — порядок максимального размера сообщения для очереди. Значение **8 КБ** является дефолтным лимитом ядра Linux (`kernel.msgmax`). Эти тесты показывают максимальные накладные расходы на системные вызовы и переключение контекста из-за огромного количества итераций.
*   **«Средний размер» (1 МБ и 8 МБ)** — порядок размера буфера FIFO. Значение **1 МБ** соответствует системному лимиту Linux на максимальный размер пайпа для пользователя.
*   **«Большой размер» (32 МБ и 64 МБ)** — существенно больше размера буфера FIFO. Буферы в 32–64 раза превышают лимит пайпа. На этих объемах FIFO упирается в ограничение ядра и вынужден дробить данные, в то время как разделяемая память (SHM) полностью раскрывает свой потенциал, работая напрямую с RAM без промежуточного копирования данных через ядро.

<h2 style="text-align: center;"> Таблицы всех замеров времени (в секундах) </h2>

<h3 style="text-align: center;">SHM (Shared Memory)</h3>

<table border="1" style="border-collapse: collapse; border: 2px solid;">
  <thead>
    <tr>
      <th colspan="2" style="border-right: 2px solid;">64 мб</th>
      <th colspan="2" style="border-right: 2px solid;">32 мб</th>
      <th colspan="2" style="border-right: 2px solid;">8 мб</th>
      <th colspan="2" style="border-right: 2px solid;">1 мб</th>
      <th colspan="2" style="border-right: 2px solid;">64 кб</th>
      <th colspan="2">8 кб</th>
    </tr>
    <tr>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>

  </thead>
  <tbody>
    <tr>
      <td>1</td><td style="border-right: 2px solid;">2,34929</td><td>1</td><td style="border-right: 2px solid;">2,34864</td><td>1</td><td style="border-right: 2px solid;">2,61912</td><td>1</td><td style="border-right: 2px solid;">2,52939</td><td>1</td><td style="border-right: 2px solid;">2,20435</td><td>1</td><td>5,56269</td>
    </tr>
    <tr>
      <td>2</td><td style="border-right: 2px solid;">3,31639</td><td>2</td><td style="border-right: 2px solid;">3,22345</td><td>2</td><td style="border-right: 2px solid;">3,93374</td><td>2</td><td style="border-right: 2px solid;">3,17177</td><td>2</td><td style="border-right: 2px solid;">2,01614</td><td>2</td><td>5,71988</td>
    </tr>
    <tr>
      <td>3</td><td style="border-right: 2px solid;">2,32457</td><td>3</td><td style="border-right: 2px solid;">2,4042</td><td>3</td><td style="border-right: 2px solid;">3,13453</td><td>3</td><td style="border-right: 2px solid;">2,69282</td><td>3</td><td style="border-right: 2px solid;">2,0391</td><td>3</td><td>5,82955</td>
    </tr>
    <tr>
      <td>4</td><td style="border-right: 2px solid;">2,43797</td><td>4</td><td style="border-right: 2px solid;">2,23617</td><td>4</td><td style="border-right: 2px solid;">3,47708</td><td>4</td><td style="border-right: 2px solid;">2,25545</td><td>4</td><td style="border-right: 2px solid;">2,27912</td><td>4</td><td>6,22455</td>
    </tr>
    <tr>
      <td>5</td><td style="border-right: 2px solid;">2,39052</td><td>5</td><td style="border-right: 2px solid;">2,45423</td><td>5</td><td style="border-right: 2px solid;">3,679</td><td>5</td><td style="border-right: 2px solid;">2,17147</td><td>5</td><td style="border-right: 2px solid;">2,03927</td><td>5</td><td>5,53963</td>
    </tr>
    <tr style="border-top: 2px solid;">
      <td><b>среднее</b></td><td style="border-right: 2px solid;"><b>2,563748</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>2,53338</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>3,368694</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>2,56418</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>2,115596</b></td><td><b>среднее</b></td><td><b>5,77526</b></td>
    </tr>
  </tbody>
</table>

<table border="1" style="border-collapse: collapse; border: 2px solid;">
  <thead>
    <tr>
      <th style="font-weight: bold; border-right: 2px solid; padding: 4px 10px;">buffer size</th>
      <td style="font-weight: normal; padding: 4px 10px;">64 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">32 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">8 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">1мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">64 кб</td>
      <td style="font-weight: normal; padding: 4px 10px;">8 кб</td>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th style="font-weight: bold; border-right: 2px solid; padding: 4px 10px;">time, secs</th>
      <td style="padding: 4px 10px;">2,5637</td>
      <td style="padding: 4px 10px;">2,5333</td>
      <td style="padding: 4px 10px;">3,3687</td>
      <td style="padding: 4px 10px;">2,5642</td>
      <td style="padding: 4px 10px;">2,1156</td>
      <td style="padding: 4px 10px;">5,7753</td>
    </tr>
  </tbody>
</table>

<div style="text-align: center;">
    <img src="pictures/SHM.png" alt="График SHM" style="width: 40%; max-width: 50%; height: auto;">>
</div>

<h3 style="text-align: center;"> SYS V QUEUE </h3>

<table border="1" style="border-collapse: collapse; border: 2px solid;">
  <thead>
    <tr>
      <th colspan="2" style="border-right: 2px solid;">64 мб</th>
      <th colspan="2" style="border-right: 2px solid;">32 мб</th>
      <th colspan="2" style="border-right: 2px solid;">8 мб</th>
      <th colspan="2" style="border-right: 2px solid;">1 мб</th>
      <th colspan="2" style="border-right: 2px solid;">64 кб</th>
      <th colspan="2">8 кб</th>
    </tr>
    <tr>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>1</td><td style="border-right: 2px solid;">5,15321</td><td>1</td><td style="border-right: 2px solid;">5,15095</td><td>1</td><td style="border-right: 2px solid;">5,16418</td><td>1</td><td style="border-right: 2px solid;">4,16288</td><td>1</td><td style="border-right: 2px solid;">2,67582</td><td>1</td><td>2,57165</td>
    </tr>
    <tr>
      <td>2</td><td style="border-right: 2px solid;">6,50066</td><td>2</td><td style="border-right: 2px solid;">6,76811</td><td>2</td><td style="border-right: 2px solid;">6,53036</td><td>2</td><td style="border-right: 2px solid;">4,17245</td><td>2</td><td style="border-right: 2px solid;">2,73766</td><td>2</td><td>2,63304</td>
    </tr>
    <tr>
      <td>3</td><td style="border-right: 2px solid;">6,33888</td><td>3</td><td style="border-right: 2px solid;">6,27445</td><td>3</td><td style="border-right: 2px solid;">6,23539</td><td>3</td><td style="border-right: 2px solid;">3,85657</td><td>3</td><td style="border-right: 2px solid;">2,66842</td><td>3</td><td>2,6181</td>
    </tr>
    <tr>
      <td>4</td><td style="border-right: 2px solid;">6,53537</td><td>4</td><td style="border-right: 2px solid;">6,20329</td><td>4</td><td style="border-right: 2px solid;">5,91238</td><td>4</td><td style="border-right: 2px solid;">4,55741</td><td>4</td><td style="border-right: 2px solid;">2,85823</td><td>4</td><td>2,60366</td>
    </tr>
    <tr>
      <td>5</td><td style="border-right: 2px solid;">5,86044</td><td>5</td><td style="border-right: 2px solid;">6,50255</td><td>5</td><td style="border-right: 2px solid;">6,34366</td><td>5</td><td style="border-right: 2px solid;">3,88829</td><td>5</td><td style="border-right: 2px solid;">3,5034</td><td>5</td><td>2,61765</td>
    </tr>
    <tr style="border-top: 2px solid;">
      <td><b>среднее</b></td><td style="border-right: 2px solid;"><b>6,077712</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>6,17987</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>6,037194</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>4,12752</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>2,888706</b></td><td><b>среднее</b></td><td><b>2,60882</b></td>
    </tr>
  </tbody>
</table>

<table border="1" style="border-collapse: collapse; border: 2px solid;">
  <thead>
    <tr>
      <th style="font-weight: bold; border-right: 2px solid; padding: 4px 10px;">buffer size</th>
      <td style="font-weight: normal; padding: 4px 10px;">64 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">32 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">8 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">1мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">64 кб</td>
      <td style="font-weight: normal; padding: 4px 10px;">8 кб</td>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th style="font-weight: bold; border-right: 2px solid; padding: 4px 10px;">time, secs</th>
      <td style="padding: 4px 10px;">6,0777</td>
      <td style="padding: 4px 10px;">6,1799</td>
      <td style="padding: 4px 10px;">6,0372</td>
      <td style="padding: 4px 10px;">4,1275</td>
      <td style="padding: 4px 10px;">2,8887</td>
      <td style="padding: 4px 10px;">2,6088</td>
    </tr>
  </tbody>
</table>

<div style="text-align: center;">
    <img src="pictures/SYS_V_QUEUE.png" alt="График sys v queue" style="width: 40%; max-width: 50%; height: auto;">
</div>

<h3 style="text-align: center;">FIFO</h3>

<table border="1" style="border-collapse: collapse; border: 2px solid;">
  <thead>
    <tr>
      <th colspan="2" style="border-right: 2px solid;">64 мб</th>
      <th colspan="2" style="border-right: 2px solid;">32 мб</th>
      <th colspan="2" style="border-right: 2px solid;">8 мб</th>
      <th colspan="2" style="border-right: 2px solid;">1 мб</th>
      <th colspan="2" style="border-right: 2px solid;">64 кб</th>
      <th colspan="2">8 кб</th>
    </tr>
    <tr>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
      <th>test</th><th style="border-right: 2px solid;">time, sec</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>1</td><td style="border-right: 2px solid;">4,12837</td><td>1</td><td style="border-right: 2px solid;">4,40254</td><td>1</td><td style="border-right: 2px solid;">4,05693</td><td>1</td><td style="border-right: 2px solid;">3,60645</td><td>1</td><td style="border-right: 2px solid;">2,50272</td><td>1</td><td>4,16811</td>
    </tr>
    <tr>
      <td>2</td><td style="border-right: 2px solid;">4,2676</td><td>2</td><td style="border-right: 2px solid;">4,55969</td><td>2</td><td style="border-right: 2px solid;">4,14393</td><td>2</td><td style="border-right: 2px solid;">3,59785</td><td>2</td><td style="border-right: 2px solid;">2,53361</td><td>2</td><td>4,07145</td>
    </tr>
    <tr>
      <td>3</td><td style="border-right: 2px solid;">4,20489</td><td>3</td><td style="border-right: 2px solid;">4,46566</td><td>3</td><td style="border-right: 2px solid;">3,99227</td><td>3</td><td style="border-right: 2px solid;">3,37085</td><td>3</td><td style="border-right: 2px solid;">2,68431</td><td>3</td><td>4,03362</td>
    </tr>
    <tr>
      <td>4</td><td style="border-right: 2px solid;">4,30805</td><td>4</td><td style="border-right: 2px solid;">4,43222</td><td>4</td><td style="border-right: 2px solid;">4,18025</td><td>4</td><td style="border-right: 2px solid;">3,37566</td><td>4</td><td style="border-right: 2px solid;">2,50988</td><td>4</td><td>4,18189</td>
    </tr>
    <tr>
      <td>5</td><td style="border-right: 2px solid;">4,47157</td><td>5</td><td style="border-right: 2px solid;">4,35818</td><td>5</td><td style="border-right: 2px solid;">4,04189</td><td>5</td><td style="border-right: 2px solid;">3,44547</td><td>5</td><td style="border-right: 2px solid;">2,49755</td><td>5</td><td>4,19305</td>
    </tr>
    <tr style="border-top: 2px solid;">
      <td><b>среднее</b></td><td style="border-right: 2px solid;"><b>4,276096</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>4,443658</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>4,083054</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>3,479256</b></td><td><b>среднее</b></td><td style="border-right: 2px solid;"><b>2,545614</b></td><td><b>среднее</b></td><td><b>4,129624</b></td>
    </tr>
  </tbody>
</table>

<table border="1" style="border-collapse: collapse; border: 2px solid;">
  <thead>
    <tr>
      <th style="font-weight: bold; border-right: 2px solid; padding: 4px 10px;">buffer size</th>
      <td style="font-weight: normal; padding: 4px 10px;">64 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">32 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">8 мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">1мб</td>
      <td style="font-weight: normal; padding: 4px 10px;">64 кб</td>
      <td style="font-weight: normal; padding: 4px 10px;">8 кб</td>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th style="font-weight: bold; border-right: 2px solid; padding: 4px 10px;">time, secs</th>
      <td style="padding: 4px 10px;">4,2761</td>
      <td style="padding: 4px 10px;">4,4437</td>
      <td style="padding: 4px 10px;">4,0831</td>
      <td style="padding: 4px 10px;">3,4793</td>
      <td style="padding: 4px 10px;">2,5456</td>
      <td style="padding: 4px 10px;">4,1296</td>
    </tr>
  </tbody>
</table>

<div style="text-align: center;">
    <img src="pictures/FIFO.png" alt="График FIFO" style="width: 40%; max-width: 50%; height: auto;">
</div>

## 