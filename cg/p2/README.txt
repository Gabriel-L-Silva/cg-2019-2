Autores: Gabriel Lucas da Silva e Lucas Santana Escobar

Todos os itens pedidos foram implementados. Porém, é necessário destacar alguns pontos sobre os itens A4. 

A4: A translação foi feita utilizando o transform da camera do editor, levando a mesma para a posição do objeto de foco(em relação ao eixo global). Após isso
utilizou-se o método de translação para modificar a posição em z da câmera do editor, em relação ao seu eixo local. Por fim, fez-se o dimensionamento
da câmera do editor utilizando o teorema de pitágoras, com as informações de dimensão do objeto e viewAngle. É importante destacar que o objeto
focado tem sua altura mostrada inteiramente apenas enquanto a mesma é menor que 30 unidades, depois disso o objeto não se encontra inteiramente dentro do campo
de vista da câmera do editor no eixo y.