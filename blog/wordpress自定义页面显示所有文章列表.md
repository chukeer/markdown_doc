Title: wordpress自定义页面显示所有文章列表
Date: 2016-09-06
Category: Skill
Tags: wordpress
Slug: wordpress自定义页面显示所有文章列表
Author: littlewhite

[TOC]

wordpress博客里有两种类型的网页，一种叫文章，一种叫页面（page），文章就是你发表的每篇博客所在的网页，页面就是你网站导航栏里的那些链接，比如“首页”，“关于我”这样的网页，这种网页的特点是集中展示某一类信息，比如首页展示每篇博客的摘要，“关于我”展示博主简介等等，自定义文章列表毫无疑问也是属于这一类的  

page类型的网页都是根据模板生成的，wordpress默认没有这一类模板，因此需要自己写一个PHP脚本，首先我们找到模板所在的目录，假设你的wordpress所在目录为/var/www，那么模板脚本在/var/www/wp-content/themes/your_theme，其中your_theme是你所使用的主体包，在里面建立一个文件page-allpost.php，内容如下

	<?php
		get_header();
	?>
	<style type="text/css">
	#table-allpost{border-collapse:collapse;}
	#table-allpost td,#table-allpost th{border:1px solid #98bf21;padding:3px 7px 2px 7px;text-align:center;}
	#table-allpost th{font-size:1.1em;text-align:center;padding-top:5px;padding-bottom:4px;background-color:#A7C942;color:#ffffff;}
	#table-allpost td{border:1px dotted #98bf21;}
	#table-allpost .td-left{text-align:left;}
	</style>
	<head><meta http-equiv="Content-Type" content="text/html; charset=utf-8" /></head>
	<div style="padding-bottom:10px"><strong>全部文章</strong></div>
	<div id="page-allpost">
	<table id="table-allpost">
	<tr>
	<th><strong>编号</strong></th>
	<th><strong>发布时间</strong></th>
	<th><strong>标题</strong></th>
	</tr>
	<?php 
		$Count_Posts = wp_count_posts(); $Num_Posts = $Count_Posts->publish; query_posts('posts_per_page=-1&caller_get_posts=1' );
		while ( have_posts() ) : the_post();
			$Num = sprintf("%03d", $Num_Posts);
			echo '<tr>';
			echo '<th>'.$Num.'</th>';
			echo '<td>';the_time(get_option( 'date_format' ));
			echo '</td><td class="td-left";><a href="'; the_permalink();
			echo '" title="'.esc_attr( get_the_title() ).'">'; the_title();
			echo '</a></td></tr>';
			$Num_Posts--;
		endwhile; wp_reset_query();
	 ?>
	</table>
	</div>
	<?php
		get_sidebar();
	?>
	<?php
		get_footer();
	?>
保存好之后，再去wordpress后台新建一个页面，注意不是发表文章，而是在仪表盘的“页面”一栏里选择新建页面，标题写“全部文章”，内容为空，别名（固定链接）设置为“<font color="red" >allpost</font>”，注意这里的别名必须和之前的脚本名page-allpost.php对应。点击保存，然后刷新你的站点首页，看看导航栏里是不是有了“全部文章”选项，点击进去看看是不是如下效果  

![image](http://littlewhite.us/pic/allpost_screenshot.png)
