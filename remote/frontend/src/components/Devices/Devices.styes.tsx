import styled from "styled-components";

export const Container = styled.div`
  width: 50%;
  margin-left: auto;
  margin-right: auto;
`;

export const TableTitle = styled.div`
  margin-top: 20px;

  table {
    width: 100%;
    border-collapse: collapse;
    th {
      padding: 10px;
      font-family: Arial, sans-serif;
      font-weight: bold;
      color: grey;
      text-align: center;
      width: 33%; /* Başlıkların genişliği */
      font-size: 20px;
      font-style: bold;
      border-bottom: 1px solid #ddd; /* Her satırın altında çizgi oluşturur */
    }
  }
`;

export const TableContainer = styled.div`
  table {
    width: 100%;
    border-collapse: collapse;

    th {
      padding: 10px;
      color: #333;
      font-style: normal;
      text-align: center;
      width: 33%; /* Başlıkların genişliği */
      vertical-align: middle; /* Resimlerin hücrenin ortasında olmasını sağlar */
    }

    tr {
      border-bottom: 1px solid #ddd; /* Her satırın altında çizgi oluşturur */
    }
  }

  table:hover {
    background-color: rgb(204, 255, 178);
    color: white;
  }
`;