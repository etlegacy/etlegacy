package com.karin.idTech4Amm.misc;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.util.Log;

import com.karin.idTech4Amm.sys.Constants;

import org.json.JSONObject;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

/**
 * preference backup
 */
public final class PreferenceBackup {
    private static final String XML_ROOT_NAME = "map";
    private static final String XML_ATTR_KEY_NAME = "name";
    private static final String XML_ATTR_VERSION_NAME = "ver";
    private static final String XML_VERSION = "1.0";

    public static final int RESULT_NO_ERROR = 0;
    public static final int RESULT_ERROR = 1;
    public static final int RESULT_DIFFERENT_VERSION = 2;
    public static final int RESULT_MISSING_ROOT_TAG = 3;
    public static final int RESULT_MANY_ROOT_TAG = 4;
    public static final int RESULT_ROOT_TAG_INVALID = 5;
    public static final int RESULT_DETAIL_TAG_INVALID = 6;

    private final Context m_context;
    private List<Item> items;
    private int m_lastErrno = RESULT_NO_ERROR;
    private String m_lastError = "";

    public PreferenceBackup(Context context)
    {
        m_context = context;
    }

    public int GetError(String[]...args)
    {
        int error = m_lastErrno;
        m_lastErrno = RESULT_NO_ERROR;
        if(null != args && args.length > 0 && null != args[0] && args[0].length > 0)
            args[0][0] = m_lastError;
        m_lastError = "";
        return error;
    }

    private void SetError(int errno, String error)
    {
        m_lastErrno = errno;
        if(null != error)
            m_lastError = error;
        else
        {
            if(errno == RESULT_NO_ERROR)
                m_lastError = "";
            else
                m_lastError = "Error";
        }
        Log.e(getClass().getSimpleName(), m_lastErrno + ": " + m_lastError);
    }

    public boolean Dump(OutputStream file)
    {
        if(null != items)
            items.clear();
        else
            items = new ArrayList<>();

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        Map<String, ?> all = preferences.getAll();
        Set<? extends Map.Entry<String, ?>> entries = all.entrySet();
        for (Map.Entry<String, ?> entry : entries)
        {
            items.add(new Item(entry.getKey(), entry.getValue()));
        }

        try {
            DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = builderFactory.newDocumentBuilder();
            Document document = builder.newDocument();
            TransformerFactory transformerFactory = TransformerFactory.newInstance();
            Transformer transformer = transformerFactory.newTransformer();
            transformer.setOutputProperty(OutputKeys.INDENT, "yes");
            transformer.setOutputProperty(OutputKeys.STANDALONE, "yes");
            transformer.setOutputProperty(OutputKeys.ENCODING, "utf-8");
            transformer.setOutputProperty(OutputKeys.VERSION, "1.0");
            DOMSource source = new DOMSource(document);
            Element root = document.createElement(XML_ROOT_NAME);
            root.setAttribute(XML_ATTR_VERSION_NAME, XML_VERSION);
            for (Item item : items)
            {
                item.Write(root);
            }
            document.appendChild(root);
            StreamResult result = new StreamResult(file);
            transformer.transform(source, result);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            SetError(RESULT_ERROR, e.getMessage());
            return false;
        }
    }

    public boolean Restore(InputStream file)
    {
        if(null != items)
            items.clear();
        else
            items = new ArrayList<>();

        try {
            DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = builderFactory.newDocumentBuilder();
            Document document = builder.parse(file);
            NodeList map = document.getElementsByTagName(XML_ROOT_NAME);
            if(map.getLength() == 0)
            {
                SetError(RESULT_MISSING_ROOT_TAG, String.format("The backup file missing root node named `%s`", XML_ROOT_NAME));
                return false;
            }
            if(map.getLength() > 1)
            {
                SetError(RESULT_MANY_ROOT_TAG, String.format("The backup file has too many node named `%s`", XML_ROOT_NAME));
                return false;
            }
            Node rootNode = map.item(0);
            if(!(rootNode instanceof Element))
            {
                SetError(RESULT_ROOT_TAG_INVALID, String.format("The backup file root node named `%s` is invalid", XML_ROOT_NAME));
                return false;
            }
            Element root = (Element)rootNode;
            String version = root.getAttribute(XML_ATTR_VERSION_NAME);
            if(!XML_VERSION.equals(version))
            {
                SetError(RESULT_DIFFERENT_VERSION, String.format("Current application backup version is %s, but backup file version is %s", XML_VERSION, version));
                return false;
            }
            List<Element> childNodes = FilterElementNodes(root.getChildNodes());
            for (Element child : childNodes)
            {
                Item item = new Item();
                if(item.Read(child))
                    items.add(item);
            }


            SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(m_context).edit();
            for (Item item : items) {
                item.Restore(editor);
            }
            editor.commit();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            SetError(RESULT_ERROR, e.getMessage());
            return false;
        }
    }

    private static List<Element> FilterElementNodes(NodeList childNodes)
    {
        List<Element> res = new ArrayList<>();
        for(int i = 0; i < childNodes.getLength(); i++)
        {
            Node child = childNodes.item(i);
            if(child.getNodeType() == Node.ELEMENT_NODE)
                res.add((Element) child);
        }
        return res;
    }

    private static class Item
    {
        public static final String TYPE_STRING = "string";
        public static final String TYPE_FLOAT = "float";
        public static final String TYPE_INT = "int";
        public static final String TYPE_LONG = "long";
        public static final String TYPE_BOOLEAN = "boolean";
        public static final String TYPE_STRING_SET = "set";

        public String key;
        public String type;
        public String description;
        public Object defaultValue;
        public Object value;

        public Item()
        {
        }

        public Item(String key, String type, Object defaultValue)
        {
            this.key = key;
            this.type = type;
            this.defaultValue = defaultValue;
        }

        public Item(String key, String type, String description, Object defaultValue)
        {
            this(key, type, defaultValue);
            this.description = description;
        }

        public Item(String key, Object value)
        {
            this(key, GuessType(value), null);
            this.value = value;
        }

        public static String GuessType(Object value)
        {
            if(null == value)
                return TYPE_STRING;
            if(value instanceof Boolean)
                return TYPE_BOOLEAN;
            if(value instanceof Integer)
                return TYPE_INT;
            if(value instanceof Float)
                return TYPE_FLOAT;
            if(value instanceof Long)
                return TYPE_LONG;
            if(value instanceof String)
                return TYPE_STRING;
            if(value instanceof Set)
                return TYPE_STRING_SET;
            return TYPE_STRING;
        }

        public boolean Dump(SharedPreferences preference)
        {
            if(!preference.contains(key))
                return false;
            Object val = null;
            switch(type)
            {
                case TYPE_BOOLEAN:
                    val = preference.getBoolean(key, (Boolean) defaultValue);
                    break;
                case TYPE_FLOAT:
                    val = preference.getFloat(key, (Float) defaultValue);
                    break;
                case TYPE_INT:
                    val = preference.getInt(key, (Integer) defaultValue);
                    break;
                case TYPE_LONG:
                    val = preference.getLong(key, (Long) defaultValue);
                    break;
                case TYPE_STRING_SET:
                    val = preference.getStringSet(key, (Set<String>) defaultValue);
                    break;
                case TYPE_STRING:
                    val = preference.getString(key, (String) defaultValue);
                    break;
                default:
                    type = TYPE_STRING;
                    val = preference.getString(key, null != defaultValue ? defaultValue.toString() : "");
                    break;
            }
            value = val;
            return true;
        }

        public boolean Restore(SharedPreferences.Editor preference)
        {
            if(null == value)
                return false;
            switch(type)
            {
                case TYPE_BOOLEAN:
                    preference.putBoolean(key, (Boolean) value);
                    break;
                case TYPE_FLOAT:
                    preference.putFloat(key, (Float) value);
                    break;
                case TYPE_INT:
                    preference.putInt(key, (Integer) value);
                    break;
                case TYPE_LONG:
                    preference.putLong(key, (Long) value);
                    break;
                case TYPE_STRING_SET:
                    preference.putStringSet(key, (Set<String>) value);
                    break;
                case TYPE_STRING:
                    preference.putString(key, (String) value);
                    break;
                default:
                    type = TYPE_STRING;
                    preference.putString(key, value.toString());
                    break;
            }
            return true;
        }

        public boolean Write(Element parent)
        {
            if(null == value)
                return false;
            Element element = parent.getOwnerDocument().createElement(type);
            element.setAttribute(XML_ATTR_KEY_NAME, key);
            switch(type)
            {
                case TYPE_STRING_SET:
                    Set<String> set = (Set<String>)value;
                    for (String s : set) {
                        Element str = element.getOwnerDocument().createElement(TYPE_STRING);
                        str.setTextContent(s);
                        element.appendChild(str);
                    }
                    break;
                case TYPE_BOOLEAN:
                case TYPE_FLOAT:
                case TYPE_INT:
                case TYPE_LONG:
                case TYPE_STRING:
                default:
                    type = TYPE_STRING;
                    element.setTextContent(value.toString());
                    break;
            }
            parent.appendChild(element);
            return true;
        }

        public boolean Read(Element element)
        {
            key = element.getAttribute(XML_ATTR_KEY_NAME);
            type = element.getTagName();
            switch(type)
            {
                case TYPE_STRING_SET:
                    Set<String> set = new LinkedHashSet<>();
                    List<Element> childNodes = FilterElementNodes(element.getChildNodes());
                    for (Element child : childNodes)
                        set.add(child.getTextContent());
                    value = set;
                    break;
                case TYPE_BOOLEAN:
                    value = Boolean.parseBoolean(element.getTextContent());
                    break;
                case TYPE_FLOAT:
                    value = Float.parseFloat(element.getTextContent());
                    break;
                case TYPE_INT:
                    value = Integer.parseInt(element.getTextContent());
                    break;
                case TYPE_LONG:
                    value = Long.parseLong(element.getTextContent());
                    break;
                case TYPE_STRING:
                    value = element.getTextContent();
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
}
