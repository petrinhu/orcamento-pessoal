<?xml version="1.0"?>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="xml" indent="yes"/>
    <xsl:template match="/">
        <skrooge>
            <list_unit>
                <xsl:for-each select="distinct-values(Document/BkToCstmrStmt/Stmt/Acct/@Ccy)">
                    <unit t_type="C">
                        <xsl:attribute name="id"><xsl:value-of select="." /></xsl:attribute>
                        <xsl:attribute name="t_name"><xsl:value-of select="." /></xsl:attribute>
                        <xsl:attribute name="t_symbol"><xsl:value-of select="." /></xsl:attribute>
                    </unit>
                </xsl:for-each>            
                <xsl:for-each select="distinct-values(//Amt/@Ccy)">
                    <unit t_type="C">
                        <xsl:attribute name="id"><xsl:value-of select="." /></xsl:attribute>
                        <xsl:attribute name="t_name"><xsl:value-of select="." /></xsl:attribute>
                        <xsl:attribute name="t_symbol"><xsl:value-of select="." /></xsl:attribute>
                    </unit>
                </xsl:for-each>                    
            </list_unit>
            
            <list_unitvalue>
            </list_unitvalue>

            <list_bank>
                <xsl:for-each select="distinct-values(//Ntry/../Acct/Id/IBAN)">
                    <bank>
                        <xsl:attribute name="id"><xsl:value-of select="current()" /></xsl:attribute>
                        <xsl:attribute name="t_name"><xsl:value-of select="current()" /></xsl:attribute>
                    </bank>
                </xsl:for-each>                   
            </list_bank>

            <list_account>
                <xsl:for-each select="distinct-values(//Ntry/../Acct/Id/IBAN)">
                    <account t_type="C">
                        <xsl:attribute name="id"><xsl:value-of select="current()" /></xsl:attribute>
                        <xsl:attribute name="t_name"><xsl:value-of select="current()" /></xsl:attribute>
                        <xsl:attribute name="t_number"><xsl:value-of select="current()" /></xsl:attribute>                   
                        <xsl:attribute name="rd_bank_id"><xsl:value-of select="current()" /></xsl:attribute>
                    </account>
                </xsl:for-each>                
            </list_account>
            
            <list_operation>
                <xsl:for-each select="//Ntry">
                    <operation t_mode="" r_payee_id="0" t_number="" i_group_id="0">
                        <xsl:attribute name="id"><xsl:value-of select="position()" /></xsl:attribute>
                        <xsl:choose>
                            <xsl:when test="NtryRef!=''">
                                <xsl:attribute name="t_import_id"><xsl:value-of select="NtryRef" /></xsl:attribute>
                                <xsl:attribute name="t_number"><xsl:value-of select="NtryRef" /></xsl:attribute>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:attribute name="t_import_id"><xsl:value-of select="AcctSvcrRef" /></xsl:attribute>
                                <xsl:attribute name="t_number"><xsl:value-of select="AcctSvcrRef" /></xsl:attribute>
                            </xsl:otherwise>
                        </xsl:choose>                         
                        <xsl:attribute name="d_createdate"><xsl:value-of select="BookgDt/Dt" /></xsl:attribute>
                        <xsl:attribute name="d_date"><xsl:value-of select="ValDt/Dt" /></xsl:attribute>
                        <xsl:attribute name="rc_unit_id"><xsl:value-of select="Amt/@Ccy" /></xsl:attribute>
                        <xsl:attribute name="t_comment"><xsl:value-of select="NtryDtls/TxDtls/RmtInf/Ustrd" /></xsl:attribute> 
                        <xsl:if test = "CdtDbtInd='DBIT'">           
                            <xsl:attribute name="r_payee_id"><xsl:value-of select="NtryDtls/TxDtls/RltdPties/Cdtr/Nm" /></xsl:attribute>
                        </xsl:if>
                        <xsl:if test = "CdtDbtInd='CRDT'">                        
                            <xsl:attribute name="r_payee_id"><xsl:value-of select="NtryDtls/TxDtls/RltdPties/Dbtr/Nm" /></xsl:attribute>
                        </xsl:if>                           
                        <xsl:choose>
                            <xsl:when test="../Acct/Id/IBAN!=''">
                                <xsl:attribute name="rd_account_id"><xsl:value-of select="../Acct/Id/IBAN" /></xsl:attribute>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:choose>
                                    <xsl:when test = "CdtDbtInd='DBIT'">                        
                                        <xsl:attribute name="rd_account_id"><xsl:value-of select="//DbtrAcct/Id/IBAN" /></xsl:attribute>
                                    </xsl:when>
                                    <xsl:otherwise>                        
                                        <xsl:attribute name="rd_account_id"><xsl:value-of select="//CdtrAcct/Id/IBAN" /></xsl:attribute>
                                    </xsl:otherwise>  
                                </xsl:choose>  
                            </xsl:otherwise>
                        </xsl:choose>                        
                    </operation>
                </xsl:for-each>            
            </list_operation>     
            
            <list_suboperation>
                <xsl:for-each select="//Ntry">
                    <suboperation>
                        <xsl:attribute name="id"><xsl:value-of select="position()" /></xsl:attribute>
                        <xsl:attribute name="d_date"><xsl:value-of select="ValDt/Dt" /></xsl:attribute>
                        <xsl:attribute name="rd_operation_id"><xsl:value-of select="position()" /></xsl:attribute>
                        <xsl:attribute name="t_comment"><xsl:value-of select="NtryDtls/TxDtls/RmtInf/Ustrd" /></xsl:attribute> 
                        <xsl:if test = "CdtDbtInd='DBIT'">                        
                            <xsl:attribute name="f_value">-<xsl:value-of select="Amt" /></xsl:attribute>
                        </xsl:if>
                        <xsl:if test = "CdtDbtInd='CRDT'">                        
                            <xsl:attribute name="f_value"><xsl:value-of select="Amt" /></xsl:attribute>
                        </xsl:if>                    
                    </suboperation>
                </xsl:for-each>               
            </list_suboperation>   
            
            <list_payee>
                <xsl:for-each select="//Ntry">
                    <payee>
                        <xsl:if test = "CdtDbtInd='DBIT'">           
                            <xsl:attribute name="id"><xsl:value-of select="NtryDtls/TxDtls/RltdPties/Cdtr/Nm" /></xsl:attribute>
                            <xsl:attribute name="t_name"><xsl:value-of select="NtryDtls/TxDtls/RltdPties/Cdtr/Nm" /></xsl:attribute>
                        </xsl:if>
                        <xsl:if test = "CdtDbtInd='CRDT'">                        
                            <xsl:attribute name="id"><xsl:value-of select="NtryDtls/TxDtls/RltdPties/Dbtr/Nm" /></xsl:attribute>
                            <xsl:attribute name="t_name"><xsl:value-of select="NtryDtls/TxDtls/RltdPties/Dbtr/Nm" /></xsl:attribute>
                        </xsl:if>                    
                    </payee>
                </xsl:for-each>               
            </list_payee>               
        </skrooge>
    </xsl:template>
</xsl:stylesheet>
