Autores: Gabriel Lucas da Silva e Lucas Santana Escobar

Todos os itens pedidos foram implementados. Por�m, � necess�rio destacar alguns pontos sobre os itens A1, A3 e A4. 

A1: Por n�o conseguir modificar a escala no modo de renderiza��o, o grupo decidiu por n�o modificar a escala de desenho do frustum
em nenhum momento, pois haveria uma incoer�ncia entre a renderiza��o e as arestas do volume de vista desenhado.

A3: O item foi implementado completamente, mas � necess�rio ressaltar que, por uma decis�o de projeto, a janela de preview s�
aparece quando a c�mera do objeto sendo inspecionado � a c�mera corrente.

A4: A transla��o foi feita utilizando o transform da camera do editor, levando a mesma para a posi��o do objeto de foco(em rela��o ao eixo global). Ap�s isso
utilizou-se o m�todo de transla��o para modificar a posi��o em z da c�mera do editor, em rela��o ao seu eixo local. Por fim, fez-se o dimensionamento
da c�mera do editor utilizando o teorema de pit�goras, com as informa��es de dimens�o do objeto e viewAngle. � importante destacar que o objeto
focado tem sua altura mostrada inteiramente apenas enquanto a mesma � menor que 30 unidades, depois disso o objeto n�o se encontra inteiramente dentro do campo
de vista da c�mera do editor no eixo y.