Laboratório 1 da disciplina de Sistemas Embarcados.

1. Diferentes níveis de otimização do compilador C:

  Nos  níveis de otimização baixo e médio, o código assembly gerado foi dentro do esperado, realizando as trocas de estado do LED com o devido atraso. Não houve diferença na
  temporização do atraso, uma vez que as instruções assembly geradas para o for() responsável pelo atraso no dois níveis foram as mesmas. Porém, no nível alto de otimização o
  compilador substituiu a instrução de for() , por uma instrução de salto incondicional no assembly resultante, evitando assim que a troca de estado do LED seja perceptível.
  
2. Frequência de clock (PLL) de 120MHz:

  Aumentando-se a frequência para 120MHz, a temporização do atraso não foi alterada, uma vez que na implementação é utilizado o valor da frequência configurada para definição
  do número máximo de iterações do for(). Entretanto, caso a frequência seja alterada e o número máximo de iterações do for() seja constante, ou seja, independente da frequência,
  a temporização do atraso será alterada, pois estará se alterando o tempo do ciclo de clock e por consequência o tempo de execução das instruções (neste caso, para o atraso, do
  for()) sem qualquer compensação, de forma que o atraso é inversamente proporcional à frequência de operação.
  
