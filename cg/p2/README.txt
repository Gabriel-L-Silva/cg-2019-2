Autores: Gabriel Lucas da Silva e Lucas Santana Escobar

Todos os itens pedidos foram implementados. Por�m, � necess�rio destacar alguns pontos sobre os itens A4. 

A4: A transla��o foi feita utilizando o transform da camera do editor, levando a mesma para a posi��o do objeto de foco(em rela��o ao eixo global). Ap�s isso
utilizou-se o m�todo de transla��o para modificar a posi��o em z da c�mera do editor, em rela��o ao seu eixo local. Por fim, fez-se o dimensionamento
da c�mera do editor utilizando o teorema de pit�goras, com as informa��es de dimens�o do objeto e viewAngle. � importante destacar que o objeto
focado tem sua altura mostrada inteiramente apenas enquanto a mesma � menor que 30 unidades, depois disso o objeto n�o se encontra inteiramente dentro do campo
de vista da c�mera do editor no eixo y.